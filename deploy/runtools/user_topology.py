""" Define your additional topologies here. The FireSimTopology class inherits
from UserToplogies and thus can instantiate your topology. """

from __future__ import annotations

import logging

from runtools.firesim_topology_elements import FireSimPipeNode, FireSimSwitchNode, FireSimServerNode, FireSimSuperNodeServerNode, FireSimDummyServerNode, FireSimNode

from typing import Optional, Union, Callable, Sequence, TYPE_CHECKING, cast, List, Any
if TYPE_CHECKING:
    from runtools.firesim_topology_with_passes import FireSimTopologyWithPasses

rootLogger = logging.getLogger()

class UserTopologies:
    """ A class that just separates out user-defined/configurable topologies
    from the rest of the boilerplate in FireSimTopology() """
    no_net_num_nodes: int
    custom_mapper: Optional[Union[Callable, str]]
    roots: Sequence[FireSimNode]

    def __init__(self, no_net_num_nodes: int) -> None:
        self.no_net_num_nodes = no_net_num_nodes
        self.custom_mapper = None
        self.roots = []

    def clos_m_n_r(self, m: int, n: int, r: int) -> None:
        """ DO NOT USE THIS DIRECTLY, USE ONE OF THE INSTANTIATIONS BELOW. """
        """ Clos topol where:
        m = number of root switches
        n = number of links to nodes on leaf switches
        r = number of leaf switches

        and each leaf switch has a link to each root switch.

        With the default mapping specified below, you will need:
        m switch nodes (on F1: m4.16xlarges).
        n fpga nodes (on F1: f1.16xlarges).

        TODO: improve this later to pack leaf switches with <= 4 downlinks onto
        one 16x.large.
        """

        rootswitches = [FireSimSwitchNode() for x in range(m)]
        self.roots = rootswitches
        leafswitches = [FireSimSwitchNode() for x in range(r)]
        servers = [[FireSimServerNode() for x in range(n)] for y in range(r)]
        for rswitch in rootswitches:
            rswitch.add_downlinks(leafswitches)

        for leafswitch, servergroup in zip(leafswitches, servers):
            leafswitch.add_downlinks(servergroup)

        def custom_mapper(fsim_topol_with_passes: FireSimTopologyWithPasses) -> None:
            for i, rswitch in enumerate(rootswitches):
                switch_inst_handle = fsim_topol_with_passes.run_farm.get_switch_only_host_handle()
                switch_inst = fsim_topol_with_passes.run_farm.allocate_sim_host(switch_inst_handle)
                switch_inst.add_switch(rswitch)

            for j, lswitch in enumerate(leafswitches):
                numsims = len(servers[j])
                inst_handle = fsim_topol_with_passes.run_farm.get_smallest_sim_host_handle(num_sims=numsims)
                sim_inst = fsim_topol_with_passes.run_farm.allocate_sim_host(inst_handle)
                sim_inst.add_switch(lswitch)
                for sim in servers[j]:
                    sim_inst.add_simulation(sim)

        self.custom_mapper = custom_mapper

    def clos_2_8_2(self) -> None:
        """ clos topol with:
        2 roots
        8 nodes/leaf
        2 leaves. """
        self.clos_m_n_r(2, 8, 2)

    def clos_8_8_16(self) -> None:
        """ clos topol with:
        8 roots
        8 nodes/leaf
        16 leaves. = 128 nodes."""
        self.clos_m_n_r(8, 8, 16)

    def fat_tree_4ary(self) -> None:
        # 4-ary fat tree as described in
        # http://ccr.sigcomm.org/online/files/p63-alfares.pdf
        coreswitches = [FireSimSwitchNode() for x in range(4)]
        self.roots = coreswitches
        aggrswitches = [FireSimSwitchNode() for x in range(8)]
        edgeswitches = [FireSimSwitchNode() for x in range(8)]
        servers = [FireSimServerNode() for x in range(16)]
        for switchno in range(len(coreswitches)):
            core = coreswitches[switchno]
            base = 0 if switchno < 2 else 1
            dls = list(map(lambda x: aggrswitches[x], range(base, 8, 2)))
            core.add_downlinks(dls)
        for switchbaseno in range(0, len(aggrswitches), 2):
            switchno = switchbaseno + 0
            aggr = aggrswitches[switchno]
            aggr.add_downlinks([edgeswitches[switchno], edgeswitches[switchno+1]])
            switchno = switchbaseno + 1
            aggr = aggrswitches[switchno]
            aggr.add_downlinks([edgeswitches[switchno-1], edgeswitches[switchno]])
        for edgeno in range(len(edgeswitches)):
            edgeswitches[edgeno].add_downlinks([servers[edgeno*2], servers[edgeno*2+1]])


        def custom_mapper(fsim_topol_with_passes: FireSimTopologyWithPasses) -> None:
            """ In a custom mapper, you have access to the firesim topology with passes,
            where you can access the run_farm nodes:

            Requires 2 fpga nodes w/ 8+ fpgas and 1 switch node

            To map, call add_switch or add_simulation on run farm instance
            objs in the aforementioned arrays.

            Because of the scope of this fn, you also have access to whatever
            stuff you created in the topology itself, which we expect will be
            useful for performing the mapping."""

            # map the fat tree onto one switch host instance (for core switches)
            # and two 8-sim-slot (e.g. 8-fpga) instances
            # (e.g., two pods of aggr/edge/4sims per f1.16xlarge)

            switch_inst_handle = fsim_topol_with_passes.run_farm.get_switch_only_host_handle()
            switch_inst = fsim_topol_with_passes.run_farm.allocate_sim_host(switch_inst_handle)
            for core in coreswitches:
                switch_inst.add_switch(core)

            eight_sim_host_handle = fsim_topol_with_passes.run_farm.get_smallest_sim_host_handle(num_sims=8)
            sim_hosts = [fsim_topol_with_passes.run_farm.allocate_sim_host(eight_sim_host_handle) for _ in range(2)]

            for aggrsw in aggrswitches[:4]:
                sim_hosts[0].add_switch(aggrsw)
            for aggrsw in aggrswitches[4:]:
                sim_hosts[1].add_switch(aggrsw)

            for edgesw in edgeswitches[:4]:
                sim_hosts[0].add_switch(edgesw)
            for edgesw in edgeswitches[4:]:
                sim_hosts[1].add_switch(edgesw)

            for sim in servers[:8]:
                sim_hosts[0].add_simulation(sim)
            for sim in servers[8:]:
                sim_hosts[1].add_simulation(sim)

        self.custom_mapper = custom_mapper

    def example_multilink(self) -> None:
        self.roots = [FireSimSwitchNode()]
        midswitch = FireSimSwitchNode()
        lowerlayer = [midswitch for x in range(16)]
        self.roots[0].add_downlinks(lowerlayer)
        servers = [FireSimServerNode()]
        midswitch.add_downlinks(servers)

    def example_multilink_32(self) -> None:
        self.roots = [FireSimSwitchNode()]
        midswitch = FireSimSwitchNode()
        lowerlayer = [midswitch for x in range(32)]
        self.roots[0].add_downlinks(lowerlayer)
        servers = [FireSimServerNode()]
        midswitch.add_downlinks(servers)

    def example_multilink_64(self) -> None:
        self.roots = [FireSimSwitchNode()]
        midswitch = FireSimSwitchNode()
        lowerlayer = [midswitch for x in range(64)]
        self.roots[0].add_downlinks(lowerlayer)
        servers = [FireSimServerNode()]
        midswitch.add_downlinks(servers)

    def example_cross_links(self) -> None:
        self.roots = [FireSimSwitchNode() for x in range(2)]
        midswitches = [FireSimSwitchNode() for x in range(2)]
        self.roots[0].add_downlinks(midswitches)
        self.roots[1].add_downlinks(midswitches)
        servers = [FireSimServerNode() for x in range(2)]
        midswitches[0].add_downlinks([servers[0]])
        midswitches[1].add_downlinks([servers[1]])

    def small_hierarchy_8sims(self) -> None:
        self.custom_mapper = 'mapping_use_one_8_slot_node'
        self.roots = [FireSimSwitchNode()]
        midlevel = [FireSimSwitchNode() for x in range(4)]
        servers = [[FireSimServerNode() for x in range(2)] for x in range(4)]
        self.roots[0].add_downlinks(midlevel)
        for swno in range(len(midlevel)):
            midlevel[swno].add_downlinks(servers[swno])

    def small_hierarchy_2sims(self) -> None:
        self.custom_mapper = 'mapping_use_one_8_slot_node'
        self.roots = [FireSimSwitchNode()]
        midlevel = [FireSimSwitchNode() for x in range(1)]
        servers = [[FireSimServerNode() for x in range(2)] for x in range(1)]
        self.roots[0].add_downlinks(midlevel)
        for swno in range(len(midlevel)):
            midlevel[swno].add_downlinks(servers[swno])

    def example_1config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        servers = [FireSimServerNode() for y in range(1)]
        self.roots[0].add_downlinks(servers)

    def example_2config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        servers = [FireSimServerNode() for y in range(2)]
        self.roots[0].add_downlinks(servers)

    def example_4config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        servers = [FireSimServerNode() for y in range(4)]
        self.roots[0].add_downlinks(servers)

    def example_8config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        servers = [FireSimServerNode() for y in range(8)]
        self.roots[0].add_downlinks(servers)

    def example_16config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level2switches = [FireSimSwitchNode() for x in range(2)]
        servers = [[FireSimServerNode() for y in range(8)] for x in range(2)]

        for root in self.roots:
            root.add_downlinks(level2switches)

        for l2switchNo in range(len(level2switches)):
            level2switches[l2switchNo].add_downlinks(servers[l2switchNo])

    def example_32config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level2switches = [FireSimSwitchNode() for x in range(4)]
        servers = [[FireSimServerNode() for y in range(8)] for x in range(4)]

        for root in self.roots:
            root.add_downlinks(level2switches)

        for l2switchNo in range(len(level2switches)):
            level2switches[l2switchNo].add_downlinks(servers[l2switchNo])

    def example_64config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level2switches = [FireSimSwitchNode() for x in range(8)]
        servers = [[FireSimServerNode() for y in range(8)] for x in range(8)]

        for root in self.roots:
            root.add_downlinks(level2switches)

        for l2switchNo in range(len(level2switches)):
            level2switches[l2switchNo].add_downlinks(servers[l2switchNo])

    def example_128config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level1switches = [FireSimSwitchNode() for x in range(2)]
        level2switches = [[FireSimSwitchNode() for x in range(8)] for x in range(2)]
        servers = [[[FireSimServerNode() for y in range(8)] for x in range(8)] for x in range(2)]

        self.roots[0].add_downlinks(level1switches)

        for switchno in range(len(level1switches)):
            level1switches[switchno].add_downlinks(level2switches[switchno])

        for switchgroupno in range(len(level2switches)):
            for switchno in range(len(level2switches[switchgroupno])):
                level2switches[switchgroupno][switchno].add_downlinks(servers[switchgroupno][switchno])

    def example_256config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level1switches = [FireSimSwitchNode() for x in range(4)]
        level2switches = [[FireSimSwitchNode() for x in range(8)] for x in range(4)]
        servers = [[[FireSimServerNode() for y in range(8)] for x in range(8)] for x in range(4)]

        self.roots[0].add_downlinks(level1switches)

        for switchno in range(len(level1switches)):
            level1switches[switchno].add_downlinks(level2switches[switchno])

        for switchgroupno in range(len(level2switches)):
            for switchno in range(len(level2switches[switchgroupno])):
                level2switches[switchgroupno][switchno].add_downlinks(servers[switchgroupno][switchno])

    @staticmethod
    def supernode_flatten(arr: List[Any]) -> List[Any]:
        res: List[Any] = []
        for x in arr:
            res = res + x
        return res

    def supernode_example_6config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        self.roots[0].add_downlinks([FireSimSuperNodeServerNode()])
        self.roots[0].add_downlinks([FireSimDummyServerNode() for x in range(5)])

    def supernode_example_4config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        self.roots[0].add_downlinks([FireSimSuperNodeServerNode()])
        self.roots[0].add_downlinks([FireSimDummyServerNode() for x in range(3)])

    def supernode_example_8config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        servers = UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(2)])
        self.roots[0].add_downlinks(servers)

    def supernode_example_16config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        servers = UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(4)])
        self.roots[0].add_downlinks(servers)

    def supernode_example_32config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        servers = UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(8)])
        self.roots[0].add_downlinks(servers)

    def supernode_example_64config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level2switches = [FireSimSwitchNode() for x in range(2)]
        servers = [UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(8)]) for x in range(2)]
        for root in self.roots:
            root.add_downlinks(level2switches)
        for l2switchNo in range(len(level2switches)):
            level2switches[l2switchNo].add_downlinks(servers[l2switchNo])

    def supernode_example_128config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level2switches = [FireSimSwitchNode() for x in range(4)]
        servers = [UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(8)]) for x in range(4)]
        for root in self.roots:
            root.add_downlinks(level2switches)
        for l2switchNo in range(len(level2switches)):
            level2switches[l2switchNo].add_downlinks(servers[l2switchNo])

    def supernode_example_256config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level2switches = [FireSimSwitchNode() for x in range(8)]
        servers = [UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(8)]) for x in range(8)]
        for root in self.roots:
            root.add_downlinks(level2switches)
        for l2switchNo in range(len(level2switches)):
            level2switches[l2switchNo].add_downlinks(servers[l2switchNo])

    def supernode_example_512config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level1switches = [FireSimSwitchNode() for x in range(2)]
        level2switches = [[FireSimSwitchNode() for x in range(8)] for x in range(2)]
        servers = [[UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(8)]) for x in range(8)] for x in range(2)]
        self.roots[0].add_downlinks(level1switches)
        for switchno in range(len(level1switches)):
            level1switches[switchno].add_downlinks(level2switches[switchno])
        for switchgroupno in range(len(level2switches)):
            for switchno in range(len(level2switches[switchgroupno])):
                level2switches[switchgroupno][switchno].add_downlinks(servers[switchgroupno][switchno])

    def supernode_example_1024config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level1switches = [FireSimSwitchNode() for x in range(4)]
        level2switches = [[FireSimSwitchNode() for x in range(8)] for x in range(4)]
        servers = [[UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(8)]) for x in range(8)] for x in range(4)]
        self.roots[0].add_downlinks(level1switches)
        for switchno in range(len(level1switches)):
            level1switches[switchno].add_downlinks(level2switches[switchno])
        for switchgroupno in range(len(level2switches)):
            for switchno in range(len(level2switches[switchgroupno])):
                level2switches[switchgroupno][switchno].add_downlinks(servers[switchgroupno][switchno])

    def supernode_example_deep64config(self) -> None:
        self.roots = [FireSimSwitchNode()]
        level1switches = [FireSimSwitchNode() for x in range(2)]
        level2switches = [[FireSimSwitchNode() for x in range(1)] for x in range(2)]
        servers = [[UserTopologies.supernode_flatten([[FireSimSuperNodeServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode(), FireSimDummyServerNode()] for y in range(8)]) for x in range(1)] for x in range(2)]
        self.roots[0].add_downlinks(level1switches)
        for switchno in range(len(level1switches)):
            level1switches[switchno].add_downlinks(level2switches[switchno])
        for switchgroupno in range(len(level2switches)):
            for switchno in range(len(level2switches[switchgroupno])):
                level2switches[switchgroupno][switchno].add_downlinks(servers[switchgroupno][switchno])

    def dual_example_8config(self) -> None:
        """ two separate 8-node clusters for experiments, e.g. memcached mutilate. """
        self.roots = [FireSimSwitchNode()] * 2
        servers = [FireSimServerNode() for y in range(8)]
        servers2 = [FireSimServerNode() for y in range(8)]
        self.roots[0].add_downlinks(servers)
        self.roots[1].add_downlinks(servers2)

    def triple_example_8config(self) -> None:
        """ three separate 8-node clusters for experiments, e.g. memcached mutilate. """
        self.roots = [FireSimSwitchNode()] * 3
        servers = [FireSimServerNode() for y in range(8)]
        servers2 = [FireSimServerNode() for y in range(8)]
        servers3 = [FireSimServerNode() for y in range(8)]
        self.roots[0].add_downlinks(servers)
        self.roots[1].add_downlinks(servers2)
        self.roots[2].add_downlinks(servers3)

    def no_net_config(self) -> None:
        self.roots = [FireSimServerNode() for x in range(self.no_net_num_nodes)]

    # Spins up all of the precompiled, unnetworked targets
    def all_no_net_targets_config(self) -> None:
        hwdb_entries = [
            "firesim_boom_singlecore_no_nic_l2_llc4mb_ddr3",
            "firesim_rocket_quadcore_no_nic_l2_llc4mb_ddr3",
        ]
        assert len(hwdb_entries) == self.no_net_num_nodes
        self.roots = [FireSimServerNode(hwdb_entries[x]) for x in range(self.no_net_num_nodes)]

    def fireaxe_two_node_base_config(self, hwdb_entries) -> None:
      assert len(hwdb_entries) == 2
      self.roots = [FireSimPipeNode()]
      server_edge = []
      if isinstance(self.roots[0], FireSimPipeNode):
          for (hwdb, base) in hwdb_entries.items():
              server_edge.append(FireSimServerNode(hwdb, partitioned=True, base_partition=base))
          self.roots[0].add_downlink_partition(server_edge)

    #######################################################################
    # QSFP / Fast mode / 70 MHz
    #######################################################################
    def qsfp_fastmode_one_big_rocket_70MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_one_big_rocket_soc_70MHz"  : True,
            "qsfp_fastmode_one_big_rocket_tile_70MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_two_big_rocket_70MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_two_big_rocket_soc_70MHz"  : True,
            "qsfp_fastmode_two_big_rocket_tile_70MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_four_big_rocket_70MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_four_big_rocket_soc_70MHz"  : True,
            "qsfp_fastmode_four_big_rocket_tile_70MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_eight_big_rocket_70MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_eight_big_rocket_soc_70MHz"  : True,
            "qsfp_fastmode_eight_big_rocket_tile_70MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_sixteen_big_rocket_70MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_sixteen_big_rocket_soc_70MHz"  : True,
            "qsfp_fastmode_sixteen_big_rocket_tile_70MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    #######################################################################
    # QSFP / Fast mode / 50 MHz
    #######################################################################
    def qsfp_fastmode_one_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_one_big_rocket_soc_50MHz"  : True,
            "qsfp_fastmode_one_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_two_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_two_big_rocket_soc_50MHz"  : True,
            "qsfp_fastmode_two_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_four_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_four_big_rocket_soc_50MHz"  : True,
            "qsfp_fastmode_four_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_eight_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_eight_big_rocket_soc_50MHz"  : True,
            "qsfp_fastmode_eight_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_sixteen_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_sixteen_big_rocket_soc_50MHz"  : True,
            "qsfp_fastmode_sixteen_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    #######################################################################
    # QSFP / Fast mode / 10 MHz
    #######################################################################
    def qsfp_fastmode_one_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_one_big_rocket_soc_10MHz"  : True,
            "qsfp_fastmode_one_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_two_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_two_big_rocket_soc_10MHz"  : True,
            "qsfp_fastmode_two_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_four_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_four_big_rocket_soc_10MHz"  : True,
            "qsfp_fastmode_four_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_eight_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_eight_big_rocket_soc_10MHz"  : True,
            "qsfp_fastmode_eight_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_sixteen_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_sixteen_big_rocket_soc_10MHz"  : True,
            "qsfp_fastmode_sixteen_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    #######################################################################
    # QSFP / exact mode / 50 MHz
    #######################################################################
    def qsfp_exactmode_one_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_one_big_rocket_soc_50MHz"  : True,
            "qsfp_exactmode_one_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_two_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_two_big_rocket_soc_50MHz"  : True,
            "qsfp_exactmode_two_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_four_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_four_big_rocket_soc_50MHz"  : True,
            "qsfp_exactmode_four_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_eight_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_eight_big_rocket_soc_50MHz"  : True,
            "qsfp_exactmode_eight_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_sixteen_big_rocket_50MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_sixteen_big_rocket_soc_50MHz"  : True,
            "qsfp_exactmode_sixteen_big_rocket_tile_50MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    #######################################################################
    # QSFP / exact mode / 10 MHz
    #######################################################################
    def qsfp_exactmode_one_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_one_big_rocket_soc_10MHz"  : True,
            "qsfp_exactmode_one_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_two_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_two_big_rocket_soc_10MHz"  : True,
            "qsfp_exactmode_two_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_four_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_four_big_rocket_soc_10MHz"  : True,
            "qsfp_exactmode_four_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_eight_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_eight_big_rocket_soc_10MHz"  : True,
            "qsfp_exactmode_eight_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_sixteen_big_rocket_10MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_sixteen_big_rocket_soc_10MHz"  : True,
            "qsfp_exactmode_sixteen_big_rocket_tile_10MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    #######################################################################
    # FAME 5 sweep 20/15MHz
    #######################################################################
    def qsfp_fame5_fastmode_one_large_boom_20MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_one_large_boom_soc_20MHz"  : True,
            "qsfp_fame5_fastmode_one_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_two_large_boom_20MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_two_large_boom_soc_20MHz"  : True,
            "qsfp_fame5_fastmode_two_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_three_large_boom_20MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_three_large_boom_soc_20MHz"  : True,
            "qsfp_fame5_fastmode_three_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_four_large_boom_20MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_four_large_boom_soc_20MHz"  : True,
            "qsfp_fame5_fastmode_four_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_five_large_boom_20MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_five_large_boom_soc_20MHz"  : True,
            "qsfp_fame5_fastmode_five_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_six_large_boom_20MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_six_large_boom_soc_20MHz"  : True,
            "qsfp_fame5_fastmode_six_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    #######################################################################
    # FAME 5 sweep 30/15MHz
    #######################################################################
    def qsfp_fame5_fastmode_one_large_boom_30MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_one_large_boom_soc_30MHz"  : True,
            "qsfp_fame5_fastmode_one_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_two_large_boom_30MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_two_large_boom_soc_30MHz"  : True,
            "qsfp_fame5_fastmode_two_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_three_large_boom_30MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_three_large_boom_soc_30MHz"  : True,
            "qsfp_fame5_fastmode_three_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_four_large_boom_30MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_four_large_boom_soc_30MHz"  : True,
            "qsfp_fame5_fastmode_four_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_five_large_boom_30MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_five_large_boom_soc_30MHz"  : True,
            "qsfp_fame5_fastmode_five_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fame5_fastmode_six_large_boom_30MHz_config(self) -> None:
        hwdb_entries = {
            "qsfp_fame5_fastmode_six_large_boom_soc_30MHz"  : True,
            "qsfp_fame5_fastmode_six_large_boom_tile_15MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    #######################################################################
    # fastmode vs exactmode error comparisons
    #######################################################################
    def qsfp_validate_cycleexact_rocket(self) -> None:
        hwdb_entries = {
            "qsfp_validate_cycleexact_firesim_rocket_soc"  : True,
            "qsfp_validate_cycleexact_firesim_rocket_tile" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_sha3_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_sha3_rocket_soc"   : True,
            "qsfp_exactmode_sha3_rocket_accel" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_sha3_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_sha3_rocket_soc"   : True,
            "qsfp_fastmode_sha3_rocket_accel" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_exactmode_gemmini_config(self) -> None:
        hwdb_entries = {
            "qsfp_exactmode_gemmini_rocket_soc"   : True,
            "qsfp_exactmode_gemmini_rocket_accel" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    def qsfp_fastmode_gemmini_config(self) -> None:
        hwdb_entries = {
            "qsfp_fastmode_gemmini_rocket_soc"   : True,
            "qsfp_fastmode_gemmini_rocket_accel" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

    #####################################################################
    # fpga count perf sweep
    #####################################################################
    def qsfp_3fpga_10MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_3fpga_0_10MHz"    : True,
            "qsfp_split_3fpga_1_10MHz"    : False,
            "qsfp_split_3fpga_2_10MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_3fpga_30MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_3fpga_0_30MHz"    : True,
            "qsfp_split_3fpga_1_30MHz"    : False,
            "qsfp_split_3fpga_2_30MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_3fpga_50MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_3fpga_0_50MHz"    : True,
            "qsfp_split_3fpga_1_50MHz"    : False,
            "qsfp_split_3fpga_2_50MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_3fpga_70MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_3fpga_0_70MHz"    : True,
            "qsfp_split_3fpga_1_70MHz"    : False,
            "qsfp_split_3fpga_2_70MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    ########################################

    def qsfp_4fpga_10MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_4fpga_0_10MHz"    : True,
            "qsfp_split_4fpga_1_10MHz"    : False,
            "qsfp_split_4fpga_2_10MHz"    : False,
            "qsfp_split_4fpga_3_10MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_4fpga_30MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_4fpga_0_30MHz"    : True,
            "qsfp_split_4fpga_1_30MHz"    : False,
            "qsfp_split_4fpga_2_30MHz"    : False,
            "qsfp_split_4fpga_3_30MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_4fpga_50MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_4fpga_0_50MHz"    : True,
            "qsfp_split_4fpga_1_50MHz"    : False,
            "qsfp_split_4fpga_2_50MHz"    : False,
            "qsfp_split_4fpga_3_50MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_4fpga_70MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_4fpga_0_70MHz"    : True,
            "qsfp_split_4fpga_1_70MHz"    : False,
            "qsfp_split_4fpga_2_70MHz"    : False,
            "qsfp_split_4fpga_3_70MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    #############################################

    def qsfp_5fpga_10MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_5fpga_0_10MHz"    : True,
            "qsfp_split_5fpga_1_10MHz"    : False,
            "qsfp_split_5fpga_2_10MHz"    : False,
            "qsfp_split_5fpga_3_10MHz"    : False,
            "qsfp_split_5fpga_4_10MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_5fpga_30MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_5fpga_0_30MHz"    : True,
            "qsfp_split_5fpga_1_30MHz"    : False,
            "qsfp_split_5fpga_2_30MHz"    : False,
            "qsfp_split_5fpga_3_30MHz"    : False,
            "qsfp_split_5fpga_4_30MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_5fpga_50MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_5fpga_0_50MHz"    : True,
            "qsfp_split_5fpga_1_50MHz"    : False,
            "qsfp_split_5fpga_2_50MHz"    : False,
            "qsfp_split_5fpga_3_50MHz"    : False,
            "qsfp_split_5fpga_4_50MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_5fpga_70MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_5fpga_0_70MHz"    : True,
            "qsfp_split_5fpga_1_70MHz"    : False,
            "qsfp_split_5fpga_2_70MHz"    : False,
            "qsfp_split_5fpga_3_70MHz"    : False,
            "qsfp_split_5fpga_4_70MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    #####################################################################

    def qsfp_6fpga_10MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_6fpga_0_10MHz"    : True,
            "qsfp_split_6fpga_1_10MHz"    : False,
            "qsfp_split_6fpga_2_10MHz"    : False,
            "qsfp_split_6fpga_3_10MHz"    : False,
            "qsfp_split_6fpga_4_10MHz"    : False,
            "qsfp_split_6fpga_5_10MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_6fpga_30MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_6fpga_0_30MHz"    : True,
            "qsfp_split_6fpga_1_30MHz"    : False,
            "qsfp_split_6fpga_2_30MHz"    : False,
            "qsfp_split_6fpga_3_30MHz"    : False,
            "qsfp_split_6fpga_4_30MHz"    : False,
            "qsfp_split_6fpga_5_30MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_6fpga_50MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_6fpga_0_50MHz"    : True,
            "qsfp_split_6fpga_1_50MHz"    : False,
            "qsfp_split_6fpga_2_50MHz"    : False,
            "qsfp_split_6fpga_3_50MHz"    : False,
            "qsfp_split_6fpga_4_50MHz"    : False,
            "qsfp_split_6fpga_5_50MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    def qsfp_6fpga_70MHz(self) -> None:
        hwdb_entries = {
            "qsfp_split_6fpga_0_70MHz"    : True,
            "qsfp_split_6fpga_1_70MHz"    : False,
            "qsfp_split_6fpga_2_70MHz"    : False,
            "qsfp_split_6fpga_3_70MHz"    : False,
            "qsfp_split_6fpga_4_70MHz"    : False,
            "qsfp_split_6fpga_5_70MHz"    : False
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    #########################################################################
    # 24 core config
    #########################################################################
    def fireaxe_broadwell5_config(self) -> None:
        hwdb_entries = {
            "xilinx_u250_broadwell_24_soc_30MHz": True,
            "xilinx_u250_broadwell_24_0_30MHz"  : False,
            "xilinx_u250_broadwell_24_1_30MHz"  : False,
            "xilinx_u250_broadwell_24_2_30MHz"  : False,
            "xilinx_u250_broadwell_24_3_30MHz"  : False,
          }
        for (hwdb, base) in hwdb_entries.items():
            server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
            self.roots.append(server)

    #########################################################################
    # go gc config
    #########################################################################
    def fireaxe_four_gigaboom_config(self) -> None:
        hwdb_entries = {
            "xilinx_u250_4boom_2Ml2_base_20MHz"  : True,
            "xilinx_u250_4boom_2Ml2_split_20MHz" : False
        }
        self.fireaxe_two_node_base_config(hwdb_entries)

#    ######Used only for tutorial purposes####################
#    def example_sha3hetero_2config(self):
#        self.roots= [FireSimSwitchNode()]
#        servers = [FireSimServerNode(server_hardware_config=
#                     "firesim_boom_singlecore_nic_l2_llc4mb_ddr3"),
#                   FireSimServerNode(server_hardware_config=
#                     "firesim_rocket_singlecore_sha3_nic_l2_llc4mb_ddr3")]
#        self.roots[0].add_downlinks(servers)
