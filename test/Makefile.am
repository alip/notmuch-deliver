DEFS+= -DGITHEAD=\"$(GITHEAD)\"
AM_CFLAGS= @NOTMUCH_DELIVER_CFLAGS@ $(glib_CFLAGS)

check_PROGRAMS= notmuch-lock

notmuch_lock_SOURCES= notmuch-lock.c
notmuch_lock_LDADD= $(glib_LIBS)

TESTS = nm-test.sh

nm-test.sh: notmuch-lock \
            nm-test-mail/INBOX/cur \
            nm-test-mail/INBOX/new \
            nm-test-mail/INBOX/tmp

nm-test-mail/INBOX/cur:
	mkdir -p $@

nm-test-mail/INBOX/new:
	mkdir -p $@

nm-test-mail/INBOX/tmp:
	mkdir -p $@

clean-local:
	rm -rf nm-test-mail
