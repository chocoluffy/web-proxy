# idea

- When receive and parse the response from server. 
    - for header, we read line by line, with `Rio_readlineb()`.
    - for body, we parse the body size from the header info, and read the body at once, with `Rio_readnb()`.

- [TRY]: if we can parse the header info and send that information to the client at the same time.

- use Rio instead of normal IO for read and write.
