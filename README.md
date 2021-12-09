# What is this?
This is a modified version of [GL.iNet's bleScanner tool](https://github.com/gl-inet/sdk) that sends [encoded raddec](https://github.com/reelyactive/raddec) strings to a [Pareto Anywhere](https://github.com/reelyactive/pareto-anywhere) server, instead of outputting the scan results as JSON strings. The program filters for [Minew's beacons](https://www.minew.com/) and sends only those to the server.

# How do I compile this?
Use [GL.iNet's BLE SDK](https://github.com/gl-inet/gl-ble-sdk) and follow their instructions.

# What can run this?
This has been tested on a GL-X750v2 Spitz, but the precompiled package should work just as well for the GL-X300B Collie and the GL-XE300 Puli. The code should be able to compile for any of GL.iNet's other routers that support BLE.

# How do I use this?
Install the package with "opkg install" and update raddec.conf with the IP address of your Pareto Anywhere server. After installation, simply type "bleRaddec" to run. The included init.d file allows you to have bleRaddec run automatically at boot and restart if the process is killed. 
