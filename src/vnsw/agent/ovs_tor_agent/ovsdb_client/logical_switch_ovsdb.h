/*
 * Copyright (c) 2014 Juniper Networks, Inc. All rights reserved.
 */

#ifndef SRC_VNSW_AGENT_OVS_TOR_AGENT_OVSDB_CLIENT_LOGICAL_SWITCH_OVSDB_H_
#define SRC_VNSW_AGENT_OVS_TOR_AGENT_OVSDB_CLIENT_LOGICAL_SWITCH_OVSDB_H_

#include <ovsdb_entry.h>
#include <ovsdb_object.h>
#include <ovsdb_client_idl.h>
#include <ovsdb_resource_vxlan_id.h>

class PhysicalDeviceVn;

namespace OVSDB {
class MulticastMacLocalEntry;
class LogicalSwitchEntry;
class OvsdbResourceVxLanId;

class LogicalSwitchTable : public OvsdbDBObject {
public:
    typedef std::map<struct ovsdb_idl_row *, LogicalSwitchEntry *> OvsdbIdlRowMap;
    LogicalSwitchTable(OvsdbClientIdl *idl);
    virtual ~LogicalSwitchTable();

    void OvsdbNotify(OvsdbClientIdl::Op, struct ovsdb_idl_row *);
    void OvsdbMcastLocalMacNotify(OvsdbClientIdl::Op, struct ovsdb_idl_row *);
    void OvsdbMcastRemoteMacNotify(OvsdbClientIdl::Op, struct ovsdb_idl_row *);
    void OvsdbUcastLocalMacNotify(OvsdbClientIdl::Op, struct ovsdb_idl_row *);

    KSyncEntry *Alloc(const KSyncEntry *key, uint32_t index);
    KSyncEntry *DBToKSyncEntry(const DBEntry*);
    OvsdbDBEntry *AllocOvsEntry(struct ovsdb_idl_row *row);
    DBFilterResp OvsdbDBEntryFilter(const DBEntry *entry,
                                    const OvsdbDBEntry *ovsdb_entry);
    void ProcessDeleteTableReq();

private:
    class ProcessDeleteTableReqTask : public Task {
    public:
        static const int KEntriesPerIteration = 32;
        ProcessDeleteTableReqTask(LogicalSwitchTable *table);
        virtual ~ProcessDeleteTableReqTask();

        bool Run();
        std::string Description() const {
            return "LogicalSwitchTable::ProcessDeleteTableReqTask";
        }

    private:
        LogicalSwitchTable *table_;
        KSyncEntry::KSyncEntryPtr entry_;
        DISALLOW_COPY_AND_ASSIGN(ProcessDeleteTableReqTask);
    };

    OvsdbIdlRowMap  idl_row_map_;
    DISALLOW_COPY_AND_ASSIGN(LogicalSwitchTable);
};

class LogicalSwitchEntry : public OvsdbDBEntry {
public:
    typedef std::set<struct ovsdb_idl_row *> OvsdbIdlRowList;
    enum Trace {
        ADD_REQ,
        DEL_REQ,
        ADD_ACK,
        DEL_ACK,
    };
    LogicalSwitchEntry(OvsdbDBObject *table, const std::string &name);
    LogicalSwitchEntry(OvsdbDBObject *table, const LogicalSwitchEntry *key);
    LogicalSwitchEntry(OvsdbDBObject *table,
            const PhysicalDeviceVn *entry);
    LogicalSwitchEntry(OvsdbDBObject *table,
            struct ovsdb_idl_row *entry);

    virtual ~LogicalSwitchEntry();

    Ip4Address &physical_switch_tunnel_ip();
    void AddMsg(struct ovsdb_idl_txn *);
    void ChangeMsg(struct ovsdb_idl_txn *);
    void DeleteMsg(struct ovsdb_idl_txn *);

    void OvsdbChange();

    const std::string &name() const;
    const std::string &device_name() const;
    int64_t vxlan_id() const;
    std::string tor_service_node() const;
    const IpAddress &tor_ip() const;
    const OvsdbResourceVxLanId &res_vxlan_id() const;

    bool Sync(DBEntry*);
    bool IsLess(const KSyncEntry&) const;
    std::string ToString() const {return "Logical Switch";}
    KSyncEntry* UnresolvedReference();

    bool IsLocalMacsRef() const;

    // Override Ack api to get trigger on Ack
    void Ack(bool success);

    // Override TxnDoneNoMessage to get triggers for no message
    // transaction complete
    void TxnDoneNoMessage();

private:
    void SendTrace(Trace event) const;
    void DeleteOldMcastRemoteMac();

    void ReleaseLocatorCreateReference();

    friend class LogicalSwitchTable;
    std::string name_;
    std::string device_name_;
    KSyncEntryPtr physical_switch_;
    // self ref to account for local mac from ToR, we hold
    // the reference till timeout or when all the local
    // macs are withdrawn
    KSyncEntryPtr local_mac_ref_;

    // physical_locator create ref
    KSyncEntryPtr pl_create_ref_;

    int64_t vxlan_id_;
    struct ovsdb_idl_row *mcast_local_row_;
    struct ovsdb_idl_row *mcast_remote_row_;
    OvsdbIdlRowList old_mcast_remote_row_list_;
    OvsdbIdlRowList ucast_local_row_list_;
    IpAddress tor_ip_;
    MulticastMacLocalEntry *mc_flood_entry_;
    OvsdbResourceVxLanId res_vxlan_id_;
    DISALLOW_COPY_AND_ASSIGN(LogicalSwitchEntry);
};
};

#endif //SRC_VNSW_AGENT_OVS_TOR_AGENT_OVSDB_CLIENT_LOGICAL_SWITCH_OVSDB_H_

