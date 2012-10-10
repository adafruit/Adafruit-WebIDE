#!/bin/bash
### BEGIN INIT INFO
# Provides:          adafruit_webide_node_debian_init
# Required-Start:    $remote_fs $named $syslog
# Required-Stop:     $remote_fs $named $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: DEBIAN initscript for Adafruit-WebIDE
# Description:       A web based IDE for remote development.
### END INIT INFO

# Credit to Peter Host (www.oghme.com) for the starting point of this script.
# If you wish the Daemon to be lauched at boot / stopped at shutdown :
#     INSTALL : update-rc.d scriptname defaults 
#     (UNINSTALL : update-rc.d -f  scriptname remove)

# Author: justin@adafruit.com
# ______________________________________________________________________________
PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin


DAEMON_ARGS="/usr/share/adafruit/webide/server.js"
DESC="Adafruit WebIDE server"
NODEUSER=webide:webide
LOCAL_VAR_RUN="/var/run"
NAME=adafruit-webide
DAEMON=$(which node)

# Do NOT "set -e"

[ $UID -eq "0" ] && LOCAL_VAR_RUN=/var/run # in case this script is run by root, override user setting
THIS_ARG=$0
INIT_SCRIPT_NAME=`basename $THIS_ARG`
#[ -h $THIS_ARG ] && INIT_SCRIPT_NAME=`basename $(readlink $THIS_ARG)` # in case of symlink
INIT_SCRIPT_NAME_NOEXT=${INIT_SCRIPT_NAME%.*}
PIDFILE="$LOCAL_VAR_RUN/$INIT_SCRIPT_NAME_NOEXT.pid"
SCRIPTNAME=/etc/init.d/$INIT_SCRIPT_NAME

# Exit if the package is not installed
[ -x "$DAEMON" ] ||  { echo "can't find Node.js ($DAEMON)"  >&2; exit 0; }

# Exit if the 'run' folder is not present
[ -d "$LOCAL_VAR_RUN" ] || { echo "Directory $LOCAL_VAR_RUN does not exist. Modify the '$INIT_SCRIPT_NAME_NOEXT' init.d script ($THIS_ARG) accordingly" >&2; exit 0; }

# Read configuration variable file if it is present
[ -r /etc/default/$INIT_SCRIPT_NAME ] && . /etc/default/$INIT_SCRIPT_NAME

# Load the VERBOSE setting and other rcS variables
. /lib/init/vars.sh

# Define LSB log_* functions.
# Depend on lsb-base (>= 3.0-6) to ensure that this file is present.
. /lib/lsb/init-functions


pidchildtree() {
    local _pid=$1
    local _sig=${2-TERM}
    for _child in $(ps -o pid --no-headers --ppid ${_pid}); do
        pidchildtree ${_child} ${_sig}
    done
    #kill -${_sig} ${_pid}
    echo -n  " "${_pid}
}

# uncomment to override system setting
#VERBOSE=yes

#
# Function that starts the daemon/service
#
do_start()
{
        # Return
        #   0 if daemon has been started
        #   1 if daemon was already running
        #   2 if daemon could not be started
        if [ -f $PIDFILE ]; then
             RUNIDS=$(pidchildtree `cat $PIDFILE`)
             [ "$RUNIDS" != "" ] && log_daemon_msg  "  --->  Daemon already running $DESC" "$INIT_SCRIPT_NAME_NOEXT"; return 1;
        fi

#       start-stop-daemon --start --quiet --pidfile $PIDFILE --chuid $NODEUSER --background --exec $DAEMON --test -- $DAEMON_ARGS > /dev/null \
#               || { [ "$VERBOSE" != no ] && log_daemon_msg  "  --->  Daemon already running $DESC" "$INIT_SCRIPT_NAME_NOEXT"; return 1; }

        start-stop-daemon --start --quiet --chuid $NODEUSER --make-pidfile --pidfile $PIDFILE --background --exec $DAEMON -- \
                $DAEMON_ARGS \
                || { [ "$VERBOSE" != no ] && log_daemon_msg  "  --->  could not be start $DESC" "$INIT_SCRIPT_NAME_NOEXT"; return 2; }

        # Add code here, if necessary, that waits for the process to be ready
        # to handle requests from services started subsequently which depend
        # on this one.  As a last resort, sleep for some time.

        [ "$VERBOSE" != no ] && log_daemon_msg  "  --->  started $DESC" "$INIT_SCRIPT_NAME_NOEXT"
}

#
# Function that stops the daemon/service
#
do_stop()
{
        # Return
        #   0 if daemon has been stopped
        #   1 if daemon was already stopped
        #   2 if daemon could not be stopped
        #   other if a failure occurred
        RETVAL=1

        if [ -f $PIDFILE ]; then
             #echo kill $(pidchildtree `cat $PIDFILE`)
             kill $(pidchildtree `cat $PIDFILE`) 2> /dev/null
             RETVAL="$?"
        fi
        ##start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 --pidfile $PIDFILE  --chuid $NODEUSER ##--name $DAEMON
        ###start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 --pidfile $PIDFILE  --chuid $NODEUSER --name $DAEMON

        #[ "$VERBOSE" != no ] && [ "$RETVAL" = 1 ] && log_daemon_msg  "  --->  SIGKILL failed => hardkill $DESC" "$INIT_SCRIPT_NAME_NOEXT"
        [ "$RETVAL" = 2 ] && return 2
        # Wait for children to finish too if this is a daemon that forks
        # and if the daemon is only ever run from this initscript.
        # If the above conditions are not satisfied then add some other code
        # that waits for the process to drop all resources that could be
        # needed by services started subsequently.  A last resort is to
        # sleep for some time.
        start-stop-daemon --stop --quiet --oknodo --retry=0/3/KILL/5 --pidfile $PIDFILE  --chuid $NODEUSER --exec $DAEMON -- $DAEMON_ARGS
        [ "$?" = 2 ] && return 2
        # Many daemons don't delete their pidfiles when they exit.
        rm -f $PIDFILE
        [ "$VERBOSE" != no ] && [ "$RETVAL" = 1 ] && log_daemon_msg "  --->  $DESC not running" "$INIT_SCRIPT_NAME_NOEXT"
        [ "$VERBOSE" != no -a "$RETVAL" = 0 ] && log_daemon_msg "  --->  $DESC stopped" "$INIT_SCRIPT_NAME_NOEXT"
        return "$RETVAL"
}

#
# Function that sends a SIGHUP to the daemon/service
#
do_reload() {
        #
        # If the daemon can reload its configuration without
        # restarting (for example, when it is sent a SIGHUP),
        # then implement that here.
        #
        start-stop-daemon --stop --quiet --signal 1 --pidfile $PIDFILE  --chuid $NODEUSER --name $NAME
        return 0
}

#
# Function that returns the daemon
#
do_status() {
  #
  # http://refspecs.freestandards.org/LSB_3.1.1/LSB-Core-generic/LSB-Core-generic/iniscrptact.html
  # 0 program is running or service is OK
  # 1 program is dead and /var/run pid file exists
  # (2 program is dead and /var/lock lock file exists) (not used here)
  # 3 program is not running
  # 4 program or service status is unknown

  RUNNING=$(running)

  #echo $RUNNING
  #echo pidof -x $NAME

  # $PIDFILE corresponds to a live $NAME process

  #ispidactive=$(pidof -x $NAME | grep `cat $PIDFILE 2>&1` >/dev/null 2>&1)
  ispidactive=$(pidof -x $DAEMON | grep `cat $PIDFILE 2>&1` >/dev/null 2>&1)
  ISPIDACTIVE=$?

  if [ -n "$RUNNING" ]; then
    if [ $ISPIDACTIVE ]; then
      log_success_msg "$INIT_SCRIPT_NAME_NOEXT (launched by $USER) (--chuid $NODEUSER) is running"
      exit 0
    fi
  else
    if [ -f $PIDFILE ]; then
      log_success_msg "$INIT_SCRIPT_NAME_NOEXT (launched by $USER) (--chuid $NODEUSER) is not running, phantom pidfile $PIDFILE"
      exit 1
    else
      log_success_msg "no instance launched by $USER, of $INIT_SCRIPT_NAME_NOEXT (--chuid $NODEUSER) found"
      exit 3
    fi
  fi

}

running() {
 if [ -f $PIDFILE ]; then
   RUNIDS=$(pidchildtree `cat $PIDFILE`)
   [ "$RUNIDS" != "" ] &&  echo y;
 fi

 # RUNSTAT=$(start-stop-daemon --start --quiet --pidfile $PIDFILE --chuid $NODEUSER --background --exec $DAEMON --test > /dev/null)
 # if [ "$?" = 1 ]; then
 #   echo y
 # fi
}


case "$1" in
  start)
        [ "$VERBOSE" != no ] && log_daemon_msg "Starting $DESC" "$INIT_SCRIPT_NAME_NOEXT"
        do_start
        case "$?" in
                0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
                2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
        esac
        ;;
  stop)
        [ "$VERBOSE" != no ] && log_daemon_msg "Stopping $DESC" "$INIT_SCRIPT_NAME_NOEXT"
        do_stop
        case "$?" in
                0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
                2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
        esac
        ;;
  #reload|force-reload)
        #
        # If do_reload() is not implemented then leave this commented out
        # and leave 'force-reload' as an alias for 'restart'.
        #
        #log_daemon_msg "Reloading $DESC" "$NAME"
        #do_reload
        #log_end_msg $?
        #;;
  restart|force-reload)
        #
        # If the "reload" option is implemented then remove the
        # 'force-reload' alias
        #
        log_daemon_msg "Restarting $DESC" "$INIT_SCRIPT_NAME_NOEXT"
        do_stop
        case "$?" in
          0|1)
                do_start
                case "$?" in
                        0) log_end_msg 0 ;;
                        1) log_end_msg 1 ;; # Old process is still running
                        *) log_end_msg 1 ;; # Failed to start
                esac
                ;;
          *)
                # Failed to stop
                log_end_msg 1
                ;;
        esac
        ;;
  status)
    do_status
  ;;
  *)
        #echo "Usage: $SCRIPTNAME {start|stop|restart|reload|force-reload}" >&2
        echo "Usage: $SCRIPTNAME {start|stop|restart|force-reload}" >&2
        exit 3
        ;;
esac

exit 0