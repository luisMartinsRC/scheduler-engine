package com.sos.scheduler.engine.common.system;

import org.junit.Test;

import javax.annotation.Nullable;
import java.io.File;
import java.util.Properties;

import static com.sos.scheduler.engine.common.system.OperatingSystem.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

public final class OperatingSystemTest {
    @Test public void testMakeModuleFilename() {
        String filename = operatingSystem.makeModuleFilename("xx");
        if (isAIX)
            assertThat(filename, isOneOf("libxx.a", "libxx.so"));   // IBM-Java: .a, OpenJDK: .so
        else
            assertThat(filename, equalTo(isWindows? "xx.dll" : "libxx.so"));
    }

    @Test public void testMakeModuleFilenameSo() {
        assertThat(operatingSystem.makeModuleFilenameSo("xx.a.yy"), equalTo(isWindows? "xx.a.yy.dll" : "libxx.a.yy.so"));
    }

    @Test public void testMakeExecutableFilename() {
        assertThat(operatingSystem.makeExecutableFilename("xx"), isWindows? equalTo("xx.exe") : equalTo("xx"));
    }

    @Test public void testPrependJavaLibraryPath() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        checkAppendJavaLibraryPath("old", f, a.toString() + File.pathSeparator + "old");
    }

    @Test public void testPrependJavaLibraryPathNull() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        checkAppendJavaLibraryPath(null, f, a.toString());
    }

    @Test public void testPrependJavaLibraryPathEmpty() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        checkAppendJavaLibraryPath("", f, a.toString());
    }

    @Test public void testPrependJavaLibraryPathSame1() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        String old = a.toString();
        checkAppendJavaLibraryPath(old, f, old);
    }

    @Test public void testPrependJavaLibraryPathSame2() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        String old = a.toString() + File.pathSeparator + "old";
        checkAppendJavaLibraryPath(old, f, old);
    }

//    @Test public void testDirectoryRecursivly() throws IOException {
//        File dir  = Files.createTempDir();
//        File a = new File(dir, "a");
//        assertTrue(a.mkdir());
//        File b = new File(a, "b");
//        assertTrue(b.mkdir());
//        assertTrue(a.isDirectory());
//        new FileOutputStream(new File(b, "c")).close();
//        OperatingSystem.operatingSystem.removeDirectoryRecursivly(a);
//        assertFalse(a.exists());
//        assertTrue(dir.exists());
//        OperatingSystem.operatingSystem.removeDirectoryRecursivly(dir);
//    }

    private static void checkAppendJavaLibraryPath(@Nullable String old, File added, String result) {
        Properties p = new Properties();
        p.putAll(System.getProperties());
        p.remove(javaLibraryPathPropertyName);
        System.setProperties(p);
        if (old != null)  System.setProperty(javaLibraryPathPropertyName, old);
        prependJavaLibraryPath(added);
        assertThat(System.getProperty(javaLibraryPathPropertyName), equalTo(result));
    }
}