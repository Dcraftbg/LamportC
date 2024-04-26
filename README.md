# LamportC
LamportC is a C implementation of [Lamport Signature](https://en.m.wikipedia.org/wiki/Lamport_signature)

# Special Thanks

B-Con and their [sha256 implementation](https://github.com/B-Con/crypto-algorithms)
*(If you're into cryptography and cryptographic algorithms definitely check the repo out. Amazing stuff)*

# Quick start

## Using premake5
To build the project you'll need [premake5](https://premake.github.io/)
And any C compiler and build system that is supported by premake5.
Then simply run premake5 and the build system of your choice, and build it from there. 
An example with Visual Studio 2022:
```cmd
> premake5 vs2022
```

## Compiling by hand
You can compile the project yourself using:
```sh
$ cc -o lamport -I BCon_sha256/include BCon_sha256/src/sha256.c Lamport/src/main.c Lamport/src/fileutils.c
$ ./lamport
[INFO] Signed message "Hello, I completely agree with this statement: I like pancakes"!
[INFO] Did we sign "Hello, I completely agree with this statement: I like pancakes"?
[INFO] Signature was VALID
[INFO] Did we sign "Hello, I completely agree with this statement: I DON'T like pancakes"?
[INFO] Signature was INVALID
```
# TODO:
- [ ] Test on multiple platforms - (Currently I'm only testing windows. It should work fine on linux. I'm only worried about fileutils really)
- [ ] Some useful examples
- [ ] Separating Lamport out to its own library maybe?
- [ ] Test on x86 - (I haven't tested it. Given its C it should work fine. Don't know for sure though) = Probably slow for uint64 - fix that

# Purpose
This project was mostly made because of curioucity and the fact I wanted to just check out how this algorithmn worked.
I highly recommend you try and implement this yourself or build on top of what is already here. Its fairly straightforward
and pretty simple to use too!
To give you some ideas for what it can be used for:
- Simple login system (A random access token generated and a sign with the username of the user. Every time they try to login they must provide the access token)
- Chat System (A simple verification for if a specific user sent that message or not)

Be creative and play around with it. If there's any Issues you can add them to this github page 

However, I'm not going to be accepting any Pull requests (except those who just want to fix an important bug in the code),
especially not those who try and add more dependencies to the project (OpenSSL or something similar for cryptographic and secure randomness for example - this is a toy project and isn't anything serious). 
