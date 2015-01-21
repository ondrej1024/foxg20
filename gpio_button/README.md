# GPIO button 

### Description
This utility can be used to detect button press events of a push button connected to a GPIO line and execute an arbitrary shell command.  
Short and long button press events are distinguished, so 2 different shell commands can be specified to be run at the related event.  


### Files
* gpiobuttond.c  
  Source file

* gpiobutton  
  Init script to start up the button detection automatically as a service. As default behaviour, a short button press (<3s) will 
  result in a reboot, a long button press in system shutdown.

### Installation
* Download files or clone via GIT from https://github.com/ondrej1024/foxg20 directly onto your board
<pre>
# git clone https://github.com/ondrej1024/foxg20
</pre>
* Build
<pre>
# cd foxg20/gpio_button
# gcc -o gpiobuttond gpiobuttond.c
</pre>
* Copy executable file into /usr/bin
<pre>
# cp gpiobuttond /usr/bin
</pre>
* Copy init script into /etc/init.d (Change the default GPIO pin number and shell commands to your needs.)
<pre>
# cp gpiobutton /etc/init.d
</pre>
* Start service
<pre>
# service gpiobutton start
</pre>
