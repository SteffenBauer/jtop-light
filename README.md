# jtop-light
Lightweight system monitor for the Nvidia Jetson platform

<center>
<img src="https://raw.githubusercontent.com/SteffenBauer/jtop-light/master/screens/jetson_orin.png">
<br/>
<img src="https://raw.githubusercontent.com/SteffenBauer/jtop-light/master/screens/jetson_nano.png">
</center>

## Motivation
I use the excellent [jetson_stats](https://github.com/rbonghi/jetson_stats) to monitor and control my Nvidia Jetson devices.

For some use-cases, this project is a bit too heavy-weight, and has too much overhead and installation hassles. I wanted a very light-weight no-frills system monitor with as less overhead and as easy to install as possible. That meant to rewrite the basic functionality of the `jtop` tool using C.

## Building and installing
Prerequisites:
* `build-essential`
* `libncurses-dev`

To build, just type `make`  
To install, copy the resulting binary `jltop` to a location like  
`sudo cp jltop /usr/local/bin/`  
To be able to get all system information, especially the power meters, when running as normal user, set the SUID flag:  
`sudo chmod +s /usr/local/bin/jltop`

### AGPL 3.0 License

See [LICENSE](https://github.com/SteffenBauer/jtop-light/blob/master/LICENSE)

Uses code written by [Raffaelo Bonghi](https://github.com/rbonghi/jetson_stats) released under the AGPL 3.0

Changes to the original code:
* Rewrite in the C programming language
* Selective cherry-picking to use only collecting and displaying system resources
* Changing some display elements; especially CPU utilization

Please pay attention to section 16 "Limitation of Liability." Building and installation on your own risk.
