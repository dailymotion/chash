#!/usr/bin/php
<?php
define('CHASH_NODES',    99);
define('CHASH_LOOKUPS',  1000000);
define('CHASH_ITEM',     'item-54321');
define('CHASH_BALANCE',  5);

// Load extension
dl('chash.so');

// Ring initialization
$nodes = array();
for ($index = 1; $index <= CHASH_NODES; $index ++)
{
   $nodes[sprintf('node-%02d', $index)] = 1;
}
$start = microtime(true);
for ($index = 1; $index <= 1000; $index ++)
{
    $chash   = new CHash();
    $entries = $chash->setTargets($nodes);
}
printf("Initialized %d nodes (%d entries) in %d us on average\n", CHASH_NODES, $entries, round(((microtime(true) - $start) * 1000000)) / 1000);

// List lookups
$nodes = array();
$start = microtime(true);
for ($index = 1; $index <= CHASH_LOOKUPS; $index ++)
{
    $list = $chash->lookupList("item-$index");
    $nodes[$list[0]] ++;
}
printf("\nPerformed %d list lookups in %d us on average\n", CHASH_LOOKUPS, round(((microtime(true) - $start) * 1000000) / CHASH_LOOKUPS));
ksort($nodes);
print_r($nodes);

// Balanced lookups
$nodes = array();
$start = microtime(true);
for ($index = 1; $index <= CHASH_LOOKUPS; $index ++)
{
    $nodes[$chash->lookupBalance(CHASH_ITEM, CHASH_BALANCE)] ++;
}
printf("\nPerformed %d lookups in %d us on average\n", CHASH_LOOKUPS, round(((microtime(true) - $start) * 1000000) / CHASH_LOOKUPS));
ksort($nodes);
print_r($nodes);
