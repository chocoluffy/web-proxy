# idea

- When receive and parse the response from server. 
    - for header, we read line by line, with `Rio_readlineb()`.
    - for body, we parse the body size from the header info, and read the body at once, with `Rio_readnb()`.

- [TRY]: if we can parse the header info and send that information to the client at the same time.

- use Rio instead of normal IO for read and write.

- whenver init with `char var[xx]`, note that such var refers to random memory, should mark the ending position as `\0`, such that we know where to stop, and not printing out random trash.

LFU policy:
- when frequency = 1, >= min entry in LFU table will not evict any existing entry. when frequency > 1, >= will take effect.
