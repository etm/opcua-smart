# OPCUA Ruby Bindings

Copyright (C) 2019-* Jürgen "eTM" Mangler <juergen.mangler@gmail.com>.

opcua-smart is freely distributable according to the terms of the GNU Lesser
General Public License 3.0 (see the file 'COPYING').

This code is distributed without any warranty. See the file 'COPYING' for
details.

## Installation

Dependency: https://github.com/open62541/open62541 > 0.4 (master branch as of 2019-04-26)

```sh
git clone https://github.com/open62541/open62541.git
cd open62541
mkdir build
cd build
cmake ..
ccmake ..
# Configuration, see picture below
make
sudo make install
gem install opcua
```

![ccmake Config](config.png)

## EXAMPLES

TBD. See examples subdirectory.
