package com.sos.scheduler.engine.tunnel

import akka.actor.ActorSystem
import akka.util.ByteString
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.tcp.TcpConnection
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.tunnel.TunnelIT._
import com.sos.scheduler.engine.tunnel.common.MessageTcpBridge
import com.sos.scheduler.engine.tunnel.data.{TunnelConnectionMessage, TunnelId, TunnelToken}
import java.net.InetSocketAddress
import org.scalatest.FreeSpec
import scala.concurrent.{Future, Promise, blocking}
import scala.math.{log, min, pow}
import scala.util.Random

/**
 * @author Joacim Zschimmer
 */
//FIXME Increase heap  @RunWith(classOf[JUnitRunner])
final class TunnelIT extends FreeSpec {

  private lazy val actorSystem = ActorSystem(getClass.getSimpleName)
  private lazy val tunnelHandler = new TunnelHandler(actorSystem)
  import actorSystem.dispatcher

  "Simple" in {
    val tunnel = tunnelHandler.newTunnel(TunnelId("TEST-TUNNEL"))
    val tcpServer = new TcpServer(tunnel.tunnelToken, tunnelHandler.proxyAddress)
    tcpServer.start()
    for (i ← 1 to 3) {
      val request = ByteString.fromString(s"TEST-REQUEST #$i")
      val responded = tunnel.sendRequest(request) map { response ⇒
        assert(response == requestToResponse(request, tunnel.id))
      }
      awaitResult(responded, 10.s)
    }
    assert(!tcpServer.terminatedPromise.isCompleted)
    tunnel.close()
    awaitResult(tcpServer.terminatedPromise.future, 10.s)
  }

  s"$TunnelCount tunnels, one request with $MessageSizeMaximum bytes, others with random sizes" in {
    val messageSizes = Iterator(MessageSizeMaximum, 0, 1) ++ Iterator.continually { nextRandomSize(1000*1000) }
    val tunnelsAndServers = for (i ← 1 to TunnelCount) yield {
      val id = TunnelId(i.toString)
      val tunnel = tunnelHandler.newTunnel(id)
      val tcpServer = new TcpServer(tunnel.tunnelToken, tunnelHandler.proxyAddress)
      tcpServer.start()
      tunnel.connected onSuccess { case peerAddress: InetSocketAddress ⇒
        logger.info(s"$tunnel $peerAddress")
      }
      tunnel → tcpServer
    }
    val tunnelRuns = for ((tunnel, _) ← tunnelsAndServers) yield Future {
      blocking {
        val stepSize = min(Iterations, 1000)
        for (i ← 0 until Iterations by stepSize) {
          val m = Stopwatch.measureTime(stepSize, "request") {
            val request = ByteString.fromArray(Array.fill[Byte](messageSizes.next())(i.toByte))
            val responded = tunnel.sendRequest(request) map { response ⇒
              if (!byteStringsFastEqual(response, requestToResponse(request, tunnel.id))) fail("Response is not as expected")
            }
            awaitResult(responded, 10.s)
          }
          logger.info(s"$tunnel: $i requests, $m")
        }
      }
    }
    awaitResult(Future.sequence(tunnelRuns), 1.h)
    for ((tunnel, tcpServer) ← tunnelsAndServers) {
      tunnel.close()
      awaitResult(tcpServer.terminatedPromise.future, 10.s)
    }
    tunnelHandler.close()
  }

  "tunnelHandler.close" in {
    tunnelHandler.close()
  }
}

object TunnelIT {
  private val TunnelCount = 4
  private val Iterations = 100
  private val MessageSizeMaximum = MessageTcpBridge.MessageSizeMaximum - 100 // requestToResponse adds some bytes
  private val logger = Logger(getClass)

  private def nextRandomSize(n: Int) = pow(2, log(n) / log(2) * Random.nextDouble()).toInt
  private def byteStringsFastEqual(a: ByteString, b: ByteString) = java.util.Arrays.equals(a.toArray[Byte], b.toArray[Byte])

  private def requestToResponse(request: ByteString, id: TunnelId): ByteString =
    request ++ ByteString.fromString(s" RESPONSE FROM $id")

  private class TcpServer(tunnelToken: TunnelToken, masterAddress: InetSocketAddress) extends Thread {
    // Somewhat unusual, the server connects the client and sends a TunnelConnectionMessage, before acting as a usual server.
    val tunnelId = tunnelToken.id
    setName(s"TCP Server $tunnelId")
    val terminatedPromise = Promise[Unit]()

    override def run(): Unit =
      try {
        val connection = TcpConnection.connect(masterAddress)
        connection.sendMessage(TunnelConnectionMessage(tunnelToken).toByteString)
        for (request ← (Iterator.continually { connection.receiveMessage() } takeWhile { _.nonEmpty }).flatten) {
          val response = requestToResponse(ByteString.fromByteBuffer(request), tunnelId)
          logger.debug(s"$tunnelId")
          connection.sendMessage(response)
        }
        connection.close()
        terminatedPromise.success(())
      }
      catch {
        case t: Throwable ⇒ terminatedPromise.failure(t)
      }
  }
}
