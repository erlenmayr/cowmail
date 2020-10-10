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

The cryptography is based on libsodium. For details, see /docs.

## Install

Debian, Ubuntu etc.:

```
$ apt install gnome-devel libsodium-dev
$ meson build
$ ninja -C build
$ ninja -C build install
```

