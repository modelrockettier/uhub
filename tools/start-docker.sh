#!/bin/sh

die() {
    echo "$2" >&2
    exit $1
}

set -e

# If uhub.conf is missing, run the first time setup.
# Otherwise, the user has already configured uhub, so don't change anything.
if [ ! -f "/conf/uhub.conf" ]; then
    # Check for misconfiguration
    if [ -f "/etc/uhub/uhub.conf" ]; then
        die 1 "ERROR: The container expects the uhub configuration in /conf, not /etc/uhub"
    fi

    echo "--- Starting uHub setup ---"
    echo

    if [ ! -f "/conf/users.db" ]; then
        ADMIN=${UHUB_ADMIN:-admin}
        PASS=${UHUB_PASS:-$(pwgen -s 12 1)}

        uhub-passwd /conf/users.db create || \
            die $? "Failed to create user database"
        uhub-passwd /conf/users.db add "${ADMIN}" "${PASS}" admin || \
            die $? "Failed to create admin user"

        printf '%s\n' \
            "Username: ${ADMIN}" \
            "Password: ${PASS}" \
            "" \
            "To change, run the following (container must be running):" \
            "docker exec container_name \\" \
            "  uhub-passwd /conf/users.db pass \"${ADMIN}\" \"new-password\"" \
        | tee /conf/admin.txt
    fi

    echo

    # Have to do this since busybox cp doesn't support --no-clobber
    for src in conf/*; do
        dest="/${src}"
        if [ ! -f "${dest}" ]; then
            cp -v "${src}" "${dest}" || \
                die $? "Failed to create '${dest}'"
        fi
    done

    echo
    echo "--- Finished uHub setup ---"
    echo
fi

exec /app/bin/uhub -c /conf/uhub.conf $UHUBOPT "$@"

# vi: ft=sh ts=4 sw=4 tw=0 ai si et :
