#!/bin/sh

UPGRADE_FILENAME=upgrade.pkg
BACKUP_FILENAME=backup.pkg
LOGFILENAME=log/uhab.log

upgrade_system() {
   echo "Upgrading system ..." 
 
   # Backup config
   mv conf conf.old
   mv icons icons.old

   unzip -o $UPGRADE_FILENAME
   rm $UPGRADE_FILENAME
 
   # Restore config
   cp -r conf.old/* conf
   cp -r icons.old/* icons
   rm -rf conf.old
   rm -rf icons.old
  
   echo "System upgrade done" >> $LOGFILENAME
}

restore_system() {
   echo "Restoring system ..."
   unzip -o $BACKUP_FILENAME
   rm $BACKUP_FILENAME
   echo "System restore done" >> $LOGFILENAME
}

factory_reset() {
   rm -rf conf
   cp -r conf-default conf
   rm factory_reset
}


## Inifinite running loop ##
while [ 1 ]; do

   if [ -f "factory_reset" ]; then
      factory_reset
   fi

   if [ -f $UPGRADE_FILENAME ]; then
      upgrade_system
   fi

   if [ -f $BACKUP_FILENAME ]; then
      restore_system
   fi

   if [ -f "network_interfaces" ]; then
      mv -f network_interfaces /etc/network/interfaces
      /etc/init.d/networking restart &
   fi

   ./uhab 

done

