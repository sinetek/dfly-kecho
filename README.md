# dfly-kecho
Example of a Sample Echo Pseudo-Device Driver for DragonFlyBSD 4.X

With this driver loaded try:

  # echo -n "Test Data" > /dev/echo
  # cat /dev/echo
  Opened device "echo" successfully.
  Test Data
  Closing device "echo".
