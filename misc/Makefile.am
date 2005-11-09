EXTRA_DIST = prt-get_complete prt-get.aliases prt-get.deplist prt-get.conf

sysconf_DATA = prt-get.conf

install-data-local:
	$(INSTALL) -D -m 644 $(srcdir)/prt-get.deplist \
		$(DESTDIR)$(localstatedir)/lib/pkg/prt-get.deplist
	$(INSTALL) -D -m 644 $(srcdir)/prt-get.aliases \
		$(DESTDIR)$(localstatedir)/lib/pkg/prt-get.aliases
#	$(INSTALL) -D -m 644 $(srcdir)/prt-get.conf \
#		$(DESTDIR)$(sysconfdir)/prt-get.conf

uninstall-local:
	rm $(DESTDIR)$(localstatedir)/lib/pkg/prt-get.deplist
	rm $(DESTDIR)$(localstatedir)/lib/pkg/prt-get.aliases
#	rm $(DESTDIR)$(sysconfigdir)/etc/prt-get.conf
