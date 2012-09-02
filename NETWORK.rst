Network support
...............

AXE can emulate an ethernet PHY supporting the Media Independent Interface
(MII). The emulated PHY passes ethernet frames to and from a virtual TAP
networking device on the host. Usually root privileges are required to create
and configure the TAP device. The steps needed to configure the TAP device
depend on the operating system you are using.

An ethernet PHY instance is created using the --ethernet-phy option. The
following port arguments are mandatory regardless of the operating system:

* tx_clk=
* tx_en=
* txd=
* rx_clk=
* rx_dv=
* rx_er=
* rxd=

Windows
=======

Not yet supported.

OS X
====
OS X does not ship with a TUN/TAP driver. OS X TUN/TAP drivers can be
installed from http://tuntaposx.sourceforge.net.

When run with no additional arguments AXE will try and open the first
available TAP device, starting with tap0. You can specify a device to use
with the -ifname=<name> option. The network interface is created when the
device is opened and it can be configured using ifconfig. For example:

ifconfig tap0 <host ip address> up

Linux
=====
Modern versions of the Linux kernel come with a TUN/TAP driver.

When run with no additional arguments AXE will try and create a new TAP
device. You can specify a device to use with the -ifname=<name> option. This
can be used to open an existing TAP device created by, for example,
tunctl (from UML utilities) or openvpn --mktun (from OpenVPN).

The TAP interface can be configured using ifconfig. For example:

ifconfig tap0 <host-ip address> up

It is also possible to add the TAP interface to an ethernet bridge. This can
be used to connect two instances of AXE or to allow AXE to communicate with
devices on a physical network. Details of setting up ethernet bridges are
beyond the scope of this document.
