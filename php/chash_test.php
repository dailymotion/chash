#!/usr/bin/php
<?php
// Defines
define('TARGETS',       100);
define('CANDIDATES',    200000);
define('SERIALIZEPATH', '/tmp/chash.serialize');

// Helper functions
$time_start   = 0;
$test_steps   = 0;
$test_status  = 0;
$test_message = '';

function test_start($title)
{
    global $test_steps, $test_status, $test_message, $time_start;

    print str_pad($title . ' ', 50, '.') . ' ';
    $test_steps   = 0;
    $test_status  = 0;
    $test_message = '';
    $time_start   = microtime(true);
}
function test_step($status, $message = '')
{
    global $test_steps, $test_status, $test_message;

    $test_steps ++;
    if (! $test_status && $status)
    {
        $test_status = $status;
        if ($message != '')
        {
            $test_message = $message;
        }
    }
}
function test_end($message)
{
    global $test_steps, $test_status, $test_message, $time_start;

    $time_spent = (microtime(true) - $time_start) * 1000;
    if ($test_status)
    {
        print 'fail (' . $test_status;
    }
    else
    {
        print 'ok (' . sprintf('%.3fms', $time_spent);
        if ($test_steps > 1)
        {
            print ' - ' . sprintf('%.3fms/step', $time_spent / $test_steps);
        }
    }
    if (! $test_status && $test_message == '' && $message != '')
    {
        $test_message = $message;
    }
    if ($test_message != '')
    {
        print ' - ' . $test_message;
    }
    print ")\n";
}

// Load extension and create CHash instance
dl('chash.so');
$chash = new CHash();
$chash->useExceptions(false);

// Execute tests
print "\n";

test_start('addTarget');
for ($index = 1; $index <= TARGETS; $index ++)
{
   test_step($chash->addTarget(sprintf('target%03d', $index)));
}
test_step($chash->getTargetsCount() == TARGETS ? 0 : -1, 'invalid targets count ' . $chash->getTargetsCount());
test_end('added ' . $chash->getTargetsCount() . ' targets');

test_start('removeTarget');
for ($index = 10; $index < 30; $index ++)
{
    test_step($chash->removeTarget(sprintf('target%03d', $index)));
}
test_step($chash->getTargetsCount() == (TARGETS - 20) ? 0 : -1, 'invalid targets count ' . $chash->getTargetsCount());
test_end('targets count is now ' . $chash->getTargetsCount());

test_start('clearTargets');
test_step($chash->clearTargets(), 'targets count is still ' . $chash->getTargetsCount());
test_end('');

test_start('setTargets');
$targets = array();
for ($index = 1; $index <= TARGETS; $index ++)
{
    $targets[sprintf('target%03d', $index)] = 1;
}
test_step(($count = $chash->setTargets($targets)) < 0 ? $count : 0);
test_end('set ' . $count . ' targets');

test_start('serialize');
test_step(($serialized1 = $chash->serialize()) == '' ? -1 : 0);
test_end('serialized size is ' . strlen($serialized1) . ' bytes');

$chash = new CHash();
test_start('unserialize');
test_step(($count = $chash->unserialize($serialized1)) < 0 ? $count : 0);
test_end('continuum count is ' . $count);

test_start('serialize coherency');
test_step(($serialized2 = $chash->serialize()) == '' ? -1 : 0);
test_step($serialized1 != $serialized2 ? -1 : 0);
test_end('');

test_start('serializeToFile');
@unlink(SERIALIZEPATH);
test_step(($size1 = $chash->serializeToFile(SERIALIZEPATH)) < 0 ? $size1 : 0);
test_end('serialized size is ' . $size1 . ' bytes');

$chash = new CHash();
test_start('unserializeFromFile');
test_step(($count = $chash->unserializeFromFile(SERIALIZEPATH)) < 0 ? $count : 0);
test_end('continuum count is ' . $count);

test_start('file serialize coherency');
test_step(($serialized2 = $chash->serialize()) == '' ? -1 : 0);
test_step($serialized1 != $serialized2 ? -1 : 0);
test_end('');

test_start('lookupList');
$lookups = array();
for ($index = 0; $index < CANDIDATES; $index ++)
{
    $targets = $chash->lookupList(sprintf('candidate%07d', $index));
    test_step(count($targets) != 1 ? -1 : 0);
    @$lookups[$targets[0]] ++;
}
$mean = 0; $deviation = 0;
foreach ($lookups as $value)
{
    $mean += $value;
}
$mean = $mean / count($lookups);
foreach ($lookups as $value)
{
    $deviation += pow($value - $mean, 2);
}
test_end('deviation is ' . sprintf('%.2f', sqrt($deviation / count($lookups))));

test_start('lookupBalance');
$lookups = array();
for ($index = 0; $index < CANDIDATES; $index ++)
{
    $target = $chash->lookupBalance('candidate001', 10);
    test_step($target == '' ? -1 : 0);
    @$lookups[$target] ++;
}
$mean = 0; $deviation = 0;
foreach ($lookups as $value)
{
    $mean += $value;
}
$mean = $mean / count($lookups);
foreach ($lookups as $value)
{
    $deviation += pow($value - $mean, 2);
}
test_end('deviation is ' . sprintf('%.2f', sqrt($deviation / count($lookups))));

print "\n";
