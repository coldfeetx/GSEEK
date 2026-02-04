CC =gcc
LIBFLAGS =`pkg-config --cflags --libs poppler-glib gtk+-3.0`
CFLAGS =$(LIBFLAGS)
LDFLAGS =$(LIBFLAGS) -lm

EXE = gseek

OBJECTS = main_ui.o basic_page.o parse_contents.o content_page.o properties_page.o result_window.o app_sideoptions.o app_fileops.o content_search.o regexp.o shexp.o utils.o gtk_common.o 


$(EXE): $(OBJECTS)
	$(CC) -o $(EXE) $(OBJECTS) $(LDFLAGS)


$(OBJECTS): common.h

main_ui.o: app_sideoptions.h basic_page.h content_page.h result_window.h

basic_page.o: parse_contents.h

parse_contents.o: content_table.h utils.h gtk_common.h 

content_page.o: gtk_common.h parse_contents.h

properties.o: utils.h gtk_common.h gtk_time.h

result_window.o: utils.h result_db.h filematches.h

app_sideoptions.o: utils.h gtk_common.h app_externs.h nftw.h app_fileops.h parse_contents.h properties_page.h filematches.h

app_fileops.o: utils.h gtk_common.h app_externs.h parse_contents.h nftw.h content_search.h

content_search.o: utils.h content_search.h

regexp.o: gtk_common.h regexp.h

shexp.o: gtk_common.h

gtk_common.o: utils.h


.PHONY: format clean

format:
	indent *.h
	indent *.c
	rm *.h~
	rm *.c~
	./cxref.sh

clean:
	rm -f $(EXE) *.o *.so
