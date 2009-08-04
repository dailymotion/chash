#!/usr/bin/python

import unittest
import chash
import os

class TestCHash(unittest.TestCase):

    def test_add_target(self):
        c = chash.CHash()
        self.failUnlessEqual(c.add_target("192.168.0.1"), None)
        self.failUnlessEqual(c.count_targets(), 1)

    def test_set_targets(self):
        c = chash.CHash()
        self.failUnlessEqual(c.set_targets({"192.168.0.1" : 2, "192.168.0.2" : 2, "192.168.0.3" : 2,} ), 3)
        self.failUnlessEqual(c.count_targets(), 3)

        self.failUnlessRaises(TypeError, c.set_targets, "9")

        self.failUnlessRaises(TypeError, c.set_targets, {3 : 2, "192.168.0.2" : 2, "192.168.0.3" : 2,})

    def test_clear_targets(self):
        c = chash.CHash()
        c.add_target("192.168.0.1")
        c.add_target("192.168.0.2")
        c.add_target("192.168.0.3")
        self.failUnlessEqual(c.count_targets(), 3)
        self.failUnlessEqual(c.clear_targets(), None)
        self.failUnlessEqual(c.count_targets(), 0)

    def test_remove_target(self):
        c = chash.CHash()
        c.add_target("192.168.0.1")
        c.add_target("192.168.0.2")
        c.add_target("192.168.0.3")
        self.failUnlessEqual(c.count_targets(), 3)
        self.failUnlessEqual(c.remove_target("192.168.0.1"), None)
        self.failUnlessEqual(c.count_targets(), 2)
        self.failUnlessRaises(chash.CHashError, c.remove_target, "192.168.0.1")
        self.failUnlessEqual(c.count_targets(), 2)
        self.failUnlessEqual(c.remove_target("192.168.0.2"), None)
        self.failUnlessEqual(c.count_targets(), 1)
        self.failUnlessRaises(chash.CHashError, c.remove_target, "192.168.0.2")
        self.failUnlessEqual(c.count_targets(), 1)
        self.failUnlessEqual(c.remove_target("192.168.0.3"), None)
        self.failUnlessEqual(c.count_targets(), 0)

    def test_count_targets(self):
        c = chash.CHash()
        self.failUnlessEqual(c.count_targets(), 0)
        self.failUnlessEqual(c.add_target("192.168.0.1"), None)
        self.failUnlessEqual(c.count_targets(), 1)
        self.failUnlessEqual(c.add_target("192.168.0.2"), None)
        self.failUnlessEqual(c.count_targets(), 2)

    def test_lookup_list(self):
        c = chash.CHash()
        c.add_target("192.168.0.1")
        c.add_target("192.168.0.2")
        c.add_target("192.168.0.3")
        c.add_target("192.168.0.4")

        self.failUnlessEqual(c.lookup_list("1"), ["192.168.0.1"])
        self.failUnlessEqual(c.lookup_list("1", 1), ["192.168.0.1"])
        self.failUnlessEqual(c.lookup_list("1", 2), ["192.168.0.1", "192.168.0.3"])
        self.failUnlessEqual(c.lookup_list("1", 3), ["192.168.0.1", "192.168.0.3", "192.168.0.2"])
        self.failUnlessEqual(c.lookup_list("2"), ["192.168.0.1"])
        self.failUnlessEqual(c.lookup_list("3"), ["192.168.0.4"])
        self.failUnlessEqual(c.lookup_list("4"), ["192.168.0.4"])

    def test_lookup_balance(self):
        c = chash.CHash()
        c.add_target("192.168.0.1")
        c.add_target("192.168.0.2")
        c.add_target("192.168.0.3")
        c.add_target("192.168.0.4")

        self.failUnlessEqual(c.lookup_balance("1"), "192.168.0.1")
# FIXME
#        self.failUnlessEqual(c.lookup_balance("1", 1), "192.168.0.1")
#        self.failUnlessEqual(c.lookup_balance("1", 2), "192.168.0.3")
#        self.failUnlessEqual(c.lookup_balance("1", 3), "192.168.0.2")
        self.failUnlessEqual(c.lookup_balance("2"), "192.168.0.1")
        self.failUnlessEqual(c.lookup_balance("3"), "192.168.0.4")
        self.failUnlessEqual(c.lookup_balance("4"), "192.168.0.4")

    def test_serialize(self):
        c = chash.CHash()
        c.add_target("192.168.0.1")
        c.add_target("192.168.0.2")
        c.add_target("192.168.0.3")
        c.add_target("192.168.0.4")
        cs = c.serialize()

        c2 = chash.CHash()
        c2.unserialize(cs)
        self.failUnlessEqual(c2.count_targets(), 4)
        self.failUnlessEqual(c2.lookup_balance("1"), "192.168.0.1")
        self.failUnlessEqual(c2.lookup_balance("2"), "192.168.0.1")
        self.failUnlessEqual(c2.lookup_balance("3"), "192.168.0.4")
        self.failUnlessEqual(c2.lookup_balance("4"), "192.168.0.4")

    def test_serialize_file(self):
        csf = "test.cs"

        c = chash.CHash()
        c.add_target("192.168.0.1")
        c.add_target("192.168.0.2")
        c.add_target("192.168.0.3")
        c.add_target("192.168.0.4")
        cs = c.serialize_to_file(csf)
        self.failUnlessEqual(os.path.exists(csf), True)

        c2 = chash.CHash()
        c2.unserialize_from_file(csf)
        os.remove(csf)
        self.failUnlessEqual(c2.count_targets(), 4)
        self.failUnlessEqual(c2.lookup_balance("1"), "192.168.0.1")
        self.failUnlessEqual(c2.lookup_balance("2"), "192.168.0.1")
        self.failUnlessEqual(c2.lookup_balance("3"), "192.168.0.4")
        self.failUnlessEqual(c2.lookup_balance("4"), "192.168.0.4")

    def test_usage(self):
        c = chash.CHash()
        c.add_target("192.168.0.1")
        c.add_target("192.168.0.2")
        self.failUnlessEqual(c.lookup_balance("1"), "192.168.0.1")

        c.add_target("192.168.0.3")
        self.failUnlessEqual(c.lookup_balance("9"), "192.168.0.3")

        c.remove_target("192.168.0.3")
        self.failUnlessEqual(c.lookup_balance("9"), "192.168.0.1")

        c.remove_target("192.168.0.1")
        self.failUnlessEqual(c.lookup_balance("9"), "192.168.0.2")

        c.remove_target("192.168.0.2")
        self.failUnlessRaises(chash.CHashError, c.lookup_balance, "9")

        c.add_target("192.168.0.2")
        c.add_target("192.168.0.1")
        self.failUnlessEqual(c.lookup_balance("9"), "192.168.0.1")

if __name__ == '__main__':
    unittest.main()
