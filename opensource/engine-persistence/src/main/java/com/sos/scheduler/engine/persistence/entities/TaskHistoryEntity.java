package com.sos.scheduler.engine.persistence.entities;

import com.sos.scheduler.engine.data.scheduler.ClusterMemberId;
import com.sos.scheduler.engine.data.scheduler.SchedulerId;

import javax.annotation.Nullable;
import javax.persistence.*;
import java.util.Date;

import static com.google.common.base.Strings.emptyToNull;
import static com.google.common.base.Strings.nullToEmpty;
import static com.sos.scheduler.engine.persistence.SchedulerDatabases.idForDatabase;
import static com.sos.scheduler.engine.persistence.SchedulerDatabases.schedulerIdFromDatabase;
import static javax.persistence.TemporalType.TIMESTAMP;

@Entity
@Table(name="SCHEDULER_HISTORY")
public class TaskHistoryEntity {
    public static final String schedulerDummyJobPath = "(Spooler)";
    
    @Id private int id;
    @Column(name="spooler_id") private String schedulerId;
    @Column(name="cluster_member_id") private String clusterMemberId;
    @Column(name="job_name") private String jobPath;
    @Column(name="start_time") @Temporal(TIMESTAMP) private Date startTime;
    @Column(name="end_time") @Temporal(TIMESTAMP) private Date endTime;
    @Column(name="cause") private String cause;
    @Column(name="steps") private Integer steps;
    @Column(name="error_code") private String errorCode;
    @Column(name="error_text") private String errorText;

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public SchedulerId getSchedulerId() {
        return schedulerIdFromDatabase(schedulerId);
    }

    public void setSchedulerId(SchedulerId id) {
        this.schedulerId = idForDatabase(id);
    }

    public ClusterMemberId getClusterMemberId() {
        return new ClusterMemberId(nullToEmpty(clusterMemberId));
    }

    public void setClusterMemberId(ClusterMemberId clusterMemberId) {
        this.clusterMemberId = emptyToNull(clusterMemberId.asString());
    }

    public String getJobPath() {
        return jobPath.equals(schedulerDummyJobPath)? "" : "/" + jobPath;
    }

    public void setJobPath(String p) {
        this.jobPath = p.isEmpty()? schedulerDummyJobPath : p.replaceFirst("^/", "");
    }

    @Override public String toString() {
        return "schedulerId=" + getSchedulerId() +
                " clusterMemberId=" + getClusterMemberId() +
                " jobName=" + getJobPath();
    }

    @Nullable public String getCause() {
        return cause;
    }

    @Nullable public Integer getSteps() {
        return steps;
    }

    public String getErrorCode() {
        return nullToEmpty(errorCode);
    }

    public String getErrorText() {
        return nullToEmpty(errorText);
    }

    public Date getStartTime() {
        return (Date)startTime.clone();
    }

    @Nullable public Date getEndTime() {
        return endTime == null? null: (Date)endTime.clone();
    }
}