# p2p-Netchat
This repository features a chat messenger with end-to-end encryption.

Following is a summary of the main features:
- [x] UDP socket communication
- [x] Symmetric encryption using AES-256
- [x] User typing information
- [x] Message notifications

## Requirements
Developed and tested on the following setup: 
- macOS High Sierra (10.13.6)
- Qt Creator 4.1.0
- Qt 5.7.0 
- Clang 7.0
- Botan 2.6.0 (Crypto and TLS for Modern C++)

## Installation
Make sure that the Botan library is successfully installed on your system. Visit https://botan.randombit.net for more information on how to install Botan.

Open _netchat.pro_ in Qt Creater and run the build process. 

## Architecture
Depicted below is the communication model:
![Communication model](https://raw.githubusercontent.com/cfanatic/p2p-netchat/master/res/architecture.png)

## Usage
[**This video**](https://codefanatic.de/git/netchat-p2p.m4v) presents the general usage and features. Please remember that only peer-to-peer connections are supported.

[![Click to open video](https://raw.githubusercontent.com/cfanatic/p2p-netchat/master/res/preview.png)](https://codefanatic.de/git/netchat-p2p.m4v)
