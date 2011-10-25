#!/bin/sh

set -e

export NOTMUCH_CONFIG=./nm-testconfig

notmuch new

./notmuch-lock -s 1000000 -- ../src/notmuch-deliver -v INBOX < testmail
