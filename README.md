# cowmail

Cowmail is a new offline messaging system using a novel addressing mechanism. Its
goal is to provide the following features:
- End-to-end encryption
- Sender anonymity
- Recipient anonymity

One of the key ideas is that messages are not sent to the recipient's server but
pulled from the sender's server. In order to pull a message, the recipient
cryptographically proves that the message is for him without revealing his
identity.

WARNING: This project is experimental and any version below 1.0 should be used
for testing purposes only.

## Cryptography

The first prototype 0.1.0 was based on libsodium.

From 0.2.0, custom cryptography is used. The implementation is based on the
GnuTLS stack.

## Install

Debian, Ubuntu etc.:

```
$ apt install gnome-devel libgnutls28-dev
$ meson build
$ ninja -C build
$ ninja -C build install
```
