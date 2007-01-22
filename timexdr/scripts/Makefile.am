# Makefile for timexdr/scripts
#

sbin_SCRIPTS = plot_timex_data		\
	       gps2gpx			\
	       gps2hst

scriptsdatadir = @SCRIPTS_DIR@

dist_scriptsdata_DATA = plot_timex_session.gri	\
		    	gps_map.gpi		\
		    	hrm_gps.gpi		\
		    	hrm_only.gpi	
		    
EXTRA_DIST = gps2gpx	\
	     gps2hst

#plot_timex_data: plot_timex_data.skel
#	cat $(srcdir)/plot_timex_data.skel | sed 
