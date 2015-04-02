if OS_WIN32
makefile_introspection_ppflags := -D OS_WIN32
else
makefile_introspection_ppflags :=
endif

@makefile_introspection@: $(top_srcdir)/Makefile.introspection.in $(top_srcdir)/build/preprocessor.py
	$(PYTHON) "$(top_srcdir)/build/preprocessor.py" \
		$(makefile_introspection_ppflags) \
		--line-endings=lf \
		--output="$(top_builddir)/Makefile.introspection" \
		"$(top_srcdir)/Makefile.introspection.in"
