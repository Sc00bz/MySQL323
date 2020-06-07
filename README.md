# MySQL323 Cracker beta 5

Warning this is very old code. It is from March 2011. Back when Visual Studio stopped letting you compile 64 bit code. So there's a Ming makefile. I really should use `uint32_t` etc and some of the `#if`'s should be for `_MSC_VER`.

This code takes advantage of MySQL323's poorly made password hashing function to get the last two characters for free. This is a speed up of 8,836 (94\*94). Note spaces are removed from the password as per spec. The key space file only defines the password prefix (ie minus the last two characters).

## Parameters
`mysql323 number_of_threads hash key_space_file [resume_code]`

## Example
`./mysql323 4 0000000000000000 keyspace.txt`
