#! /bin/bash
##################################################################################
#
# Author:        Ondrej Wisniewski
#
# Description:   Control the relais and read the inputs of the Daisy8 module.
#                One or two chained Daisy8 modules on the same Daisy connector
#                are supported.
#                Module description: http://www.acmesystems.it/DAISY-8
#
# Usage:         daisy8 <d2|d5|d11> <in0|in1|in2|in3|rl0|rl1|rl2|rl3> <on|off>
#
# Parameters:    $1 Daisy connector <d2|d5|d11>
#                $2 Input/output selection <in0|in1|in2|in3|rl0|rl1|rl2|rl3>
#                $3 Relais control <on|off>
#
# Last modified: 19/09/2013
#
##################################################################################

GPIO_SYS_PATH="/sys/class/gpio"

# GPIO Kernel Ids definition, valid for Kernel 2.6.xx series
# See http://www.acmesystems.it/DAISY-1
# TODO: add support for Kernel 3.x.y series

# GPIO Kernel Ids for D2 Daisy connector on FoxG20
D2_P2_ID=63
D2_P3_ID=62
D2_P4_ID=61
D2_P5_ID=60
D2_P6_ID=59
D2_P7_ID=58
D2_P8_ID=57
D2_P9_ID=94

# GPIO Kernel Ids for D5 Daisy connector on FoxG20
D5_P2_ID=76
D5_P3_ID=77
D5_P4_ID=80
D5_P5_ID=81
D5_P6_ID=82
D5_P7_ID=83
D5_P8_ID=84
D5_P9_ID=85

# GPIO Kernel Ids for D11 Daisy connector on FoxG20 V2
D11_P2_ID=65
D11_P3_ID=64
D11_P4_ID=66
D11_P5_ID=67


# check Kernel version, currently we are only supprting 2.6.38
KVER=$(uname -a  | awk '{print $3}')
if [ "$KVER" != "2.6.38" ]; then 
   echo " Detected Linux Kernel $KVER."
   echo " Only Kernel version 2.6.38 is currently supported, sorry."
   exit 1
fi


# print help message if parameters are missing
if [[ -z $1 || -z $2 ]]; then
   echo "Control up to 2 Daisy8 modules"
   echo "  Usage:"
   echo "  daisy8 <Daisy connector> <In/Out selection> <Relais control>"
   echo "     Daisy connector:  d2|d5|d11"
   echo "     In/Out selection: in0|in1|in2|in3|rl0|rl1|rl2|rl3"
   echo "     Relais control:   on|off"
   exit 2
fi


# check which Daisy connector to use and define 
# the related kernel Ids for each IO line
case "$1" in
   "d2" )
       P2_ID=$D2_P2_ID
       P3_ID=$D2_P3_ID
       P4_ID=$D2_P4_ID
       P5_ID=$D2_P5_ID
       P6_ID=$D2_P6_ID
       P7_ID=$D2_P7_ID
       P8_ID=$D2_P8_ID
       P9_ID=$D2_P9_ID
      ;;  
   "d5" )
       P2_ID=$D5_P2_ID
       P3_ID=$D5_P3_ID
       P4_ID=$D5_P4_ID
       P5_ID=$D5_P5_ID
       P6_ID=$D5_P6_ID
       P7_ID=$D5_P7_ID
       P8_ID=$D5_P8_ID
       P9_ID=$D5_P9_ID
      ;;  
   "d11" )
       P2_ID=$D11_P2_ID
       P3_ID=$D11_P3_ID
       P4_ID=$D11_P4_ID
       P5_ID=$D11_P5_ID
      ;;  
    * )
      echo "ERROR: wrong Daisy connector number, specify \"d2\", \"d5\" or \"d11\""
      exit 2
      ;;
esac


# check remaining input parameters and perform requested action
case "$2" in
   "rl0" )  
      # export needed GPIO line and define as output
      if [ ! -d $GPIO_SYS_PATH/gpio$P2_ID ]; then
         echo $P2_ID > $GPIO_SYS_PATH/export
         echo out > $GPIO_SYS_PATH/gpio$P2_ID/direction
         if [ $? -ne 0 ]; then 
            echo " ERROR: GPIO Id $P2_ID is already in use on your system, check Kernel configuration"
            exit 3
         fi
      fi
      # switch on/off RL0
      case "$3" in
         "on" )
            echo 1 > $GPIO_SYS_PATH/gpio$P2_ID/value
            ;;
         "off" )        
            echo 0 > $GPIO_SYS_PATH/gpio$P2_ID/value
            ;;
          * )
            echo "ERROR: wrong action for rl0, specify \"on\" or \"off\""
            ;;
      esac
      ;;

   "rl1" )  
      # export needed GPIO line and define as output
      if [ ! -d $GPIO_SYS_PATH/gpio$P3_ID ]; then
         echo $P3_ID > $GPIO_SYS_PATH/export
         echo out > $GPIO_SYS_PATH/gpio$P3_ID/direction
         if [ $? -ne 0 ]; then 
            echo " ERROR: GPIO Id $P3_ID is already in use on your system, check Kernel configuration"
            exit 3
         fi
      fi
      # switch on/off RL1
      case $3 in
         "on" )
            echo 1 > $GPIO_SYS_PATH/gpio$P3_ID/value
            ;;
         "off" )        
            echo 0 > $GPIO_SYS_PATH/gpio$P3_ID/value
            ;;
          * )
            echo "ERROR: wrong action for rl1, specify \"on\" or \"off\""
            ;;
      esac
      ;;

   "rl2" )  
      # export needed GPIO line and define as output
      if [ ! -d $GPIO_SYS_PATH/gpio$P6_ID ]; then
         echo $P6_ID > $GPIO_SYS_PATH/export
         echo out > $GPIO_SYS_PATH/gpio$P6_ID/direction
         if [ $? -ne 0 ]; then 
            echo " ERROR: GPIO Id $P6_ID is already in use on your system, check Kernel configuration"
            exit 3
         fi
      fi
      # switch on/off RL2 (RL0 on second Daisy8)
      case $3 in
         "on" )
            echo 1 > $GPIO_SYS_PATH/gpio$P6_ID/value
            ;;
         "off" )        
            echo 0 > $GPIO_SYS_PATH/gpio$P6_ID/value
            ;;
          * )
            echo "ERROR: wrong action for rl2, specify \"on\" or \"off\""
            ;;
      esac
      ;;

   "rl3" )  
      # export needed GPIO line and define as output
      if [ ! -d $GPIO_SYS_PATH/gpio$P7_ID ]; then
         echo $P7_ID > $GPIO_SYS_PATH/export
         echo out > $GPIO_SYS_PATH/gpio$P7_ID/direction
         if [ $? -ne 0 ]; then 
            echo " ERROR: GPIO Id $P7_ID is already in use on your system, check Kernel configuration"
            exit 3
         fi
      fi
      # switch on/off RL3 (RL1 on second Daisy8)
      case $3 in
         "on" )
            echo 1 > $GPIO_SYS_PATH/gpio$P7_ID/value
            ;;
         "off" )        
            echo 0 > $GPIO_SYS_PATH/gpio$P7_ID/value
            ;;
          * )
            echo "ERROR: wrong action for rl3, specify \"on\" or \"off\""
            ;;
      esac
      ;;

   "in0" )  
      # export needed GPIO line and define as intput
      if [ ! -d $GPIO_SYS_PATH/gpio$P4_ID ]; then
         echo $P4_ID > $GPIO_SYS_PATH/export
         echo in > $GPIO_SYS_PATH/gpio$P4_ID/direction
         if [ $? -ne 0 ]; then 
            echo " ERROR: GPIO Id $P4_ID is already in use on your system, check Kernel configuration"
            exit 3
         fi
      fi
      # read current state of IN0
      cat $GPIO_SYS_PATH/gpio$P4_ID/value
      ;;

   "in1" )  
      # export needed GPIO line and define as intput
      if [ ! -d $GPIO_SYS_PATH/gpio$P5_ID ]; then
         echo $P5_ID > $GPIO_SYS_PATH/export
         echo in > $GPIO_SYS_PATH/gpio$P5_ID/direction
         if [ $? -ne 0 ]; then 
            echo " ERROR: GPIO Id $P5_ID is already in use on your system, check Kernel configuration"
            exit 3
         fi
      fi
      # read current state of IN1
      cat $GPIO_SYS_PATH/gpio$P5_ID/value
      ;;

   "in2" )  
      # export needed GPIO line and define as intput
      if [ ! -d $GPIO_SYS_PATH/gpio$P8_ID ]; then
         echo $P8_ID > $GPIO_SYS_PATH/export
         echo in > $GPIO_SYS_PATH/gpio$P8_ID/direction
         if [ $? -ne 0 ]; then 
            echo " ERROR: GPIO Id $P8_ID is already in use on your system, check Kernel configuration"
            exit 3
         fi
      fi
      # read current state of IN2 (IN0 on second Daisy8)
      cat $GPIO_SYS_PATH/gpio$P8_ID/value
      ;;

   "in3" )  
      # export needed GPIO line and define as output
      if [ ! -d $GPIO_SYS_PATH/gpio$P9_ID ]; then
         echo $P9_ID > $GPIO_SYS_PATH/export
         echo in > $GPIO_SYS_PATH/gpio$P9_ID/direction
         if [ $? -ne 0 ]; then 
            echo " ERROR: GPIO Id $P9_ID is already in use on your system, check Kernel configuration"
            exit 3
         fi
      fi
      # read current state of IN3 (IN1 on second Daisy8)
      cat $GPIO_SYS_PATH/gpio$P9_ID/value
      ;;
    *)
      echo "ERROR: wrong input/output, only the following parameters are valid:"
      echo "      \"in0\", \"in1\" for inputs on first Daisy8"
      echo "      \"in2\", \"in3\" for inputs on second Daisy8"
      echo "      \"rl0\" or \"rl1\" for relais on first Daisy8"
      echo "      \"rl2\" or \"rl3\" for relais on second Daisy8"
      ;;
esac

exit 0;
