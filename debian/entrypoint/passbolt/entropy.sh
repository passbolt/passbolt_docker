function entropy_check() {
  local entropy_avail

  entropy_avail=$(cat /proc/sys/kernel/random/entropy_avail)

  if [ "$entropy_avail" -lt 2000 ]; then

    cat <<EOF
==================================================================================
  Your entropy pool is low. This situation could lead GnuPG to not
  be able to create the gpg serverkey so the container start process will hang
  until enough entropy is obtained.
  Please consider installing rng-tools and/or virtio-rng on your host as the
  preferred method to generate random numbers using a TRNG.
  If rngd (rng-tools) does not provide enough or fast enough randomness you could
  consider installing haveged as a helper to speed up this process.
  Using haveged as a replacement for rngd is not recommended. You can read more
  about this topic here: https://lwn.net/Articles/525459/
==================================================================================
EOF
  fi
}

