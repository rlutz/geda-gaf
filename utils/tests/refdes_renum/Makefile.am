AM_TESTS_ENVIRONMENT = PERL=${PERL} top_srcdir=${srcdir}/../..

check-local:
	$(AM_TESTS_ENVIRONMENT) srcdir=$(srcdir) $(SHELL) $(srcdir)/run_tests.sh

EXTRA_DIST = $(input_files) $(output_files) run_tests.sh tests.list README

input_files = \
	inputs/allnew.sch \
	inputs/allold.sch \
	inputs/file1.sch \
	inputs/file2.sch \
	inputs/file3.sch \
	inputs/more_than_10.sch \
	inputs/someold1.sch \
	inputs/someold2.sch \
	inputs/someold3.sch

output_files = \
	outputs/all_new_refdeses-allnew.sch \
	outputs/all_new_refdeses1-allnew.sch \
	outputs/all_new_refdeses2-allnew.sch \
	outputs/all_old_refdeses-allold.sch \
	outputs/all_old_refdeses1-allold.sch \
	outputs/all_old_refdeses2-allold.sch \
	outputs/multi_clear-file1.sch \
	outputs/multi_clear-file2.sch \
	outputs/multi_clear-file3.sch \
	outputs/multi_pgskip1-file1.sch \
	outputs/multi_pgskip1-file2.sch \
	outputs/multi_pgskip1-file3.sch \
	outputs/multi_pgskip2-file1.sch \
	outputs/multi_pgskip2-file2.sch \
	outputs/multi_pgskip2-file3.sch \
	outputs/multi_renum-file1.sch \
	outputs/multi_renum-file2.sch \
	outputs/multi_renum-file3.sch \
	outputs/single_clear-file1.sch \
	outputs/single_pgskip1-file1.sch \
	outputs/single_pgskip2-file1.sch \
	outputs/single_pgskip3-more_than_10.sch \
	outputs/single_renum-file1.sch \
	outputs/some_new_pgskip-someold1.sch \
	outputs/some_new_pgskip-someold2.sch \
	outputs/some_new_pgskip-someold3.sch \
	outputs/some_new_pgskip1-someold1.sch \
	outputs/some_new_pgskip1-someold2.sch \
	outputs/some_new_pgskip1-someold3.sch \
	outputs/some_new_pgskip2-someold1.sch \
	outputs/some_new_pgskip2-someold2.sch \
	outputs/some_new_pgskip2-someold3.sch \
	outputs/some_old_refdeses-someold1.sch \
	outputs/some_old_refdeses1-someold1.sch \
	outputs/some_old_refdeses2-someold1.sch
