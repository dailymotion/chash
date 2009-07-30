#!/usr/bin/python

import unittest
import chash

class TestCHash(unittest.TestCase):

    def test_add_target(self):
        c = chash.CHash()
        self.failUnlessEqual(c.add_target("192.168.0.1"), None)
        self.failUnlessEqual(c.count_targets(), 1)

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

    def test_freeze(self):
        c = chash.CHash()
        self.failUnlessEqual(c.add_target("192.168.0.1"), None)
        self.failIfEqual(c.freeze(), None)
        self.failUnlessRaises(chash.CHashError, c.add_target, "192.168.0.3")
    
    def test_unfreeze(self):
        c = chash.CHash()
        self.failUnlessEqual(c.add_target("192.168.0.1"), None)
        self.failUnlessEqual(c.count_targets(), 1)
        self.failIfEqual(c.freeze(), None)
        self.failUnlessRaises(chash.CHashError, c.add_target, "192.168.0.3")
        self.failUnlessEqual(c.unfreeze(), None)
        self.failUnlessEqual(c.add_target("192.168.0.2", 3), None)
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

if __name__ == '__main__':
    unittest.main()
