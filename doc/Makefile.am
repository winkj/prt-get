
man_MANS = prt-get.8 prt-cache.8 prt-get.conf.5

clean-local:
	rm -rf *.log *.aux *.toc *.dvi *.idx *.pdf api 

apidoc:
	doxygen

EXTRA_DIST = $(man_MANS) coding
