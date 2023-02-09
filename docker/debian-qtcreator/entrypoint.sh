#!/bin/bash
#
# This file is part of the oasys-solver-suite project:
#   https://github.com/arup-group/oasys-solver-suite

USERNAME_=${LOCAL_USER:-root}
UID_=${LOCAL_UID:-0}
GID_=${LOCAL_GID:-$UID_}

echo "127.0.0.1 ${HOSTNAME}" >> /etc/hosts

# we are running as root, if UID_ == 0, there is little we can do but
if [[ $UID_ == 0 ]] ; then
  # - inject the bashrc if missing
  if ! [[ -e /root/.bashrc ]] ; then
     echo "[entrypoint] adding config and color scheme as /root/.bash*"
     cp /data/bashrc /root/.bashrc
     cp /data/bash-ps1 /root/.bash-ps1
     chown root:root /root/.bashrc /root/.bash-ps1
     chmod 644 /root/.bashrc /root/.bash-ps1
  fi
  # - create a ccache config if missing
  if ! [[ -e /root/.ccache/ccache.conf ]] ; then
     echo "[entrypoint] adding 5G ccache limit to /root/.ccache/ccache.conf"
     mkdir -p /root/.ccache
     echo "max_size = 5.0G" > /root/.ccache/ccache.conf
  fi
  # - and then continue
  if [ "X$@" == "X" ]; then
    exec /bin/bash
  else
    exec "$@"
  fi
  exit
fi

# ... else, check if there exist a group and create it otherwise
[[ $(getent group $GID_) ]] || groupadd -g $GID_ $USERNAME_ &> /dev/null

# check if a user with id $UID_ already exists
if [[ $(id -u $UID_ &> /dev/null) ]]; then
  # unconditionally set the primary group of the user
  usermod -g $GID_ $UID_
  # rename the user if required
  [[ "$USERNAME_" == "$(id -un $UID_)" ]] || usermod -l $USERNAME_ $(id -un $UID_)
else
  # otherwise create the user
  [[ -e /home/$USERNAME_ ]] && export m_= || export m_=m
  useradd -${m_}d /home/$USERNAME_ -g $GID_ -s /bin/bash -u $UID_ $USERNAME_
fi

# give the user some sudo capabilities
if [[ $UID_ != 0 ]] ; then
  echo "$USERNAME_ ALL=(ALL) NOPASSWD:/usr/bin/apt-get" >> /etc/sudoers
  echo "$USERNAME_ ALL=(ALL) NOPASSWD:/usr/bin/apt" >> /etc/sudoers
  echo "$USERNAME_ ALL=(ALL) NOPASSWD:/usr/bin/dpkg" >> /etc/sudoers
fi

# inject the bashrc if missing
if ! [[ -e /home/$USERNAME_/.bashrc ]] ; then
   echo "[entrypoint] adding bashrc and color scheme as /home/$USERNAME_/.bash*"
   cp /data/bashrc.tpl /home/$USERNAME_/.bashrc
   cp /data/bash-ps1 /home/$USERNAME_/.bash-ps1
   chown $UID_:$GID_ /home/$USERNAME_/.bashrc /home/$USERNAME_/.bash-ps1
   chmod 644 /home/$USERNAME_/.bashrc /home/$USERNAME_/.bash-ps1
fi
# create a ccache config if missing
if ! [[ -e /home/$USERNAME_/.ccache/ccache.conf ]] ; then
   echo "[entrypoint] adding 5G ccache limit to /home/$USERNAME_/.ccache/ccache.conf"
   mkdir -p /home/$USERNAME_/.ccache
   echo "max_size = 5.0G" > /home/$USERNAME_/.ccache/ccache.conf
fi

if [ "X$@" == "X" ]; then
  exec gosu $USERNAME_ /bin/bash
else
  exec gosu $USERNAME_ "$@"
fi

