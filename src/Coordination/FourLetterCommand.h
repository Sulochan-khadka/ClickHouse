#pragma once

#include "config.h"

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>
#include <base/types.h>
#include <boost/noncopyable.hpp>

namespace DB
{

class KeeperDispatcher;

struct IFourLetterCommand;
using FourLetterCommandPtr = std::shared_ptr<DB::IFourLetterCommand>;

/// Just like zookeeper Four Letter Words commands, CH Keeper responds to a small set of commands.
/// Each command is composed of four letters, these commands are useful to monitor and issue system problems.
/// The feature is based on Zookeeper 3.5.9, details is in https://zookeeper.apache.org/doc/r3.5.9/zookeeperAdmin.html#sc_zkCommands.
/// Also we add some additional commands such as csnp, lgif etc.
struct IFourLetterCommand
{
public:
    explicit IFourLetterCommand(KeeperDispatcher & keeper_dispatcher_);

    virtual String name() = 0;
    virtual String run() = 0;

    virtual ~IFourLetterCommand();
    int32_t code();

    static String toName(int32_t code);
    static inline int32_t toCode(const String & name);

protected:
    KeeperDispatcher & keeper_dispatcher;
};

struct FourLetterCommandFactory : private boost::noncopyable
{
public:
    using Commands = std::unordered_map<int32_t, FourLetterCommandPtr>;
    using AllowList = std::vector<int32_t>;

    /// Represents '*' which is used in allow list.
    static constexpr int32_t ALLOW_LIST_ALL = 0;

    bool isKnown(int32_t code);
    bool isEnabled(int32_t code);

    FourLetterCommandPtr get(int32_t code);

    /// There is no need to make it thread safe, because registration is no initialization and get is after startup.
    void registerCommand(FourLetterCommandPtr & command);
    void initializeAllowList(KeeperDispatcher & keeper_dispatcher);

    void checkInitialization() const;
    bool isInitialized() const { return initialized; }
    void setInitialize(bool flag) { initialized = flag; }

    static FourLetterCommandFactory & instance();
    static void registerCommands(KeeperDispatcher & keeper_dispatcher);

private:
    std::atomic<bool> initialized = false;
    Commands commands;
    AllowList allow_list;
};

/**Tests if server is running in a non-error state. The server will respond with imok if it is running.
 * Otherwise it will not respond at all.
 *
 * A response of "imok" does not necessarily indicate that the server has joined the quorum,
 * just that the server process is active and bound to the specified client port.
 * Use "stat" for details on state wrt quorum and client connection information.
 */
struct RuokCommand : public IFourLetterCommand
{
    explicit RuokCommand(KeeperDispatcher & keeper_dispatcher_) : IFourLetterCommand(keeper_dispatcher_) { }

    String name() override { return "ruok"; }
    String run() override;
    ~RuokCommand() override = default;
};

/**
 * Outputs a list of variables that could be used for monitoring the health of the cluster.
 *
 * echo mntr | nc localhost 2181
 * zk_version  3.5.9
 * zk_avg_latency  0
 * zk_max_latency  0
 * zk_min_latency  0
 * zk_packets_received 70
 * zk_packets_sent 69
 * zk_outstanding_requests 0
 * zk_server_state leader
 * zk_znode_count   4
 * zk_watch_count  0
 * zk_ephemerals_count 0
 * zk_approximate_data_size    27
 * zk_open_file_descriptor_count 23    - only available on Unix platforms
 * zk_max_file_descriptor_count 1024   - only available on Unix platforms
 * zk_followers 2                      - only exposed by the Leader
 * zk_synced_followers  2              - only exposed by the Leader
 * zk_pending_syncs 0                  - only exposed by the Leader
 */
struct MonitorCommand : public IFourLetterCommand
{
    explicit MonitorCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "mntr"; }
    String run() override;
    ~MonitorCommand() override = default;
};

struct StatResetCommand : public IFourLetterCommand
{
    explicit StatResetCommand(KeeperDispatcher & keeper_dispatcher_) :
        IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "srst"; }
    String run() override;
    ~StatResetCommand() override = default;
};

/// A command that does not do anything except reply to client with predefined message.
///It is used to inform clients who execute none allow listed four letter word commands.
struct NopCommand : public IFourLetterCommand
{
    explicit NopCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "nopc"; }
    String run() override;
    ~NopCommand() override = default;
};

struct ConfCommand : public IFourLetterCommand
{
    explicit ConfCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "conf"; }
    String run() override;
    ~ConfCommand() override = default;
};

/// List full connection/session details for all clients connected to this server.
/// Includes information on numbers of packets received/sent, session id, operation latencies, last operation performed, etc...
struct ConsCommand : public IFourLetterCommand
{
    explicit ConsCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "cons"; }
    String run() override;
    ~ConsCommand() override = default;
};

/// Reset connection/session statistics for all connections.
struct RestConnStatsCommand : public IFourLetterCommand
{
    explicit RestConnStatsCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "crst"; }
    String run() override;
    ~RestConnStatsCommand() override = default;
};

/// Lists full details for the server.
struct ServerStatCommand : public IFourLetterCommand
{
    explicit ServerStatCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "srvr"; }
    String run() override;
    ~ServerStatCommand() override = default;
};

/// Lists brief details for the server and connected clients.
struct StatCommand : public IFourLetterCommand
{
    explicit StatCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "stat"; }
    String run() override;
    ~StatCommand() override = default;
};

/// Lists brief information on watches for the server.
struct BriefWatchCommand : public IFourLetterCommand
{
    explicit BriefWatchCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "wchs"; }
    String run() override;
    ~BriefWatchCommand() override = default;
};

/// Lists detailed information on watches for the server, by session.
/// This outputs a list of sessions(connections) with associated watches (paths).
/// Note, depending on the number of watches this operation may be expensive (ie impact server performance), use it carefully.
struct WatchCommand : public IFourLetterCommand
{
    explicit WatchCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "wchc"; }
    String run() override;
    ~WatchCommand() override = default;
};

/// Lists detailed information on watches for the server, by path.
/// This outputs a list of paths (znodes) with associated sessions.
/// Note, depending on the number of watches this operation may be expensive (ie impact server performance), use it carefully.
struct WatchByPathCommand : public IFourLetterCommand
{
    explicit WatchByPathCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "wchp"; }
    String run() override;
    ~WatchByPathCommand() override = default;
};

/// Lists the outstanding sessions and ephemeral nodes. This only works on the leader.
struct DumpCommand : public IFourLetterCommand
{
    explicit DumpCommand(KeeperDispatcher & keeper_dispatcher_):
        IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "dump"; }
    String run() override;
    ~DumpCommand() override = default;
};

/// Print details about serving environment
struct EnviCommand : public IFourLetterCommand
{
    explicit EnviCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "envi"; }
    String run() override;
    ~EnviCommand() override = default;
};

/// Shows the total size of snapshot and log files in bytes
struct DataSizeCommand : public IFourLetterCommand
{
    explicit DataSizeCommand(KeeperDispatcher & keeper_dispatcher_):
        IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "dirs"; }
    String run() override;
    ~DataSizeCommand() override = default;
};

/// Tests if server is running in read-only mode.
/// The server will respond with "ro" if in read-only mode or "rw" if not in read-only mode.
struct IsReadOnlyCommand : public IFourLetterCommand
{
    explicit IsReadOnlyCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "isro"; }
    String run() override;
    ~IsReadOnlyCommand() override = default;
};

struct RecoveryCommand : public IFourLetterCommand
{
    explicit RecoveryCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "rcvr"; }
    String run() override;
    ~RecoveryCommand() override = default;
};

struct ApiVersionCommand : public IFourLetterCommand
{
    explicit ApiVersionCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "apiv"; }
    String run() override;
    ~ApiVersionCommand() override = default;
};

/// Create snapshot manually
struct CreateSnapshotCommand : public IFourLetterCommand
{
    explicit CreateSnapshotCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "csnp"; }
    String run() override;
    ~CreateSnapshotCommand() override = default;
};

/** Raft log information:
 *     first_log_idx 1
 *     first_log_term   1
 *     last_log_idx 101
 *     last_log_term    1
 *     last_committed_idx   100
 *     leader_committed_log_idx 101
 *     target_committed_log_idx 101
 *     last_snapshot_idx    50
 */
struct LogInfoCommand : public IFourLetterCommand
{
    explicit LogInfoCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "lgif"; }
    String run() override;
    ~LogInfoCommand() override = default;
};

/// Request to be leader.
struct RequestLeaderCommand : public IFourLetterCommand
{
    explicit RequestLeaderCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "rqld"; }
    String run() override;
    ~RequestLeaderCommand() override = default;
};

struct RecalculateCommand : public IFourLetterCommand
{
    explicit RecalculateCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "rclc"; }
    String run() override;
    ~RecalculateCommand() override = default;
};

struct CleanResourcesCommand : public IFourLetterCommand
{
    explicit CleanResourcesCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "clrs"; }
    String run() override;
    ~CleanResourcesCommand() override = default;
};

struct FeatureFlagsCommand : public IFourLetterCommand
{
    explicit FeatureFlagsCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "ftfl"; }
    String run() override;
    ~FeatureFlagsCommand() override = default;
};

/// Yield leadership and become follower.
struct YieldLeadershipCommand : public IFourLetterCommand
{
    explicit YieldLeadershipCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "ydld"; }
    String run() override;
    ~YieldLeadershipCommand() override = default;
};

#if USE_JEMALLOC
struct JemallocDumpStats : public IFourLetterCommand
{
    explicit JemallocDumpStats(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "jmst"; }
    String run() override;
    ~JemallocDumpStats() override = default;

};

struct JemallocFlushProfile : public IFourLetterCommand
{
    explicit JemallocFlushProfile(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "jmfp"; }
    String run() override;
    ~JemallocFlushProfile() override = default;
};

struct JemallocEnableProfile : public IFourLetterCommand
{
    explicit JemallocEnableProfile(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "jmep"; }
    String run() override;
    ~JemallocEnableProfile() override = default;
};

struct JemallocDisableProfile : public IFourLetterCommand
{
    explicit JemallocDisableProfile(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "jmdp"; }
    String run() override;
    ~JemallocDisableProfile() override = default;
};
#endif

struct ProfileEventsCommand : public IFourLetterCommand
{
    explicit ProfileEventsCommand(KeeperDispatcher & keeper_dispatcher_)
        : IFourLetterCommand(keeper_dispatcher_)
    {
    }

    String name() override { return "pfev"; }
    String run() override;
    ~ProfileEventsCommand() override = default;
};

}
