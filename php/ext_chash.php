<?hh

 <<__NativeData("ZendCompat")>> class CHash {
  <<__Native("ZendCompat")>> public function useExceptions(bool $b): bool;
  <<__Native("ZendCompat")>> public function addTarget(string $target, int $weight = 1): int;
  <<__Native("ZendCompat")>> public function removeTarget(string $target): int;
  <<__Native("ZendCompat")>> public function setTargets(array $targets): int;
  <<__Native("ZendCompat")>> public function clearTargets(): int;
  <<__Native("ZendCompat")>> public function getTargetsCount(): int;
  <<__Native("ZendCompat")>> public function serialize(): string;
  <<__Native("ZendCompat")>> public function unserialize(string $serialized): int;
  <<__Native("ZendCompat")>> public function serializeToFile(string $path): int;
  <<__Native("ZendCompat")>> public function unserializeFromFile(string $path): int;
  <<__Native("ZendCompat")>> public function lookupList(string $candidate, int $count = 1): array;
  <<__Native("ZendCompat")>> public function lookupBalance(string $name, int $count = 1): string;
}

<<__NativeData("ZendCompat")>> class CHashException extends Exception {}
<<__NativeData("ZendCompat")>> class CHashMemoryException extends CHashException {}
<<__NativeData("ZendCompat")>> class CHashIOException extends CHashException {}
<<__NativeData("ZendCompat")>> class CHashInvalidParameterException extends CHashException {}
<<__NativeData("ZendCompat")>> class CHashNotFoundException extends CHashException {}
