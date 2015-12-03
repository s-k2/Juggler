OBJS := bin/document.o bin/dump.o bin/helper.o bin/metadata.o bin/render.o bin/page-add.o bin/page-remove.o bin/internal.o bin/page-rotate.o bin/put-content.o bin/rename-lexer.o bin/copy-helper.o bin/impose.o bin/export-images.o
CFLAGS := -g "-Imupdf/include"
LFLAGS := -g
LIBS := mupdf/build/debug/libmupdf.a mupdf/build/debug/libmujs.a mupdf/build/debug/libfreetype.a mupdf/build/debug/libjbig2dec.a mupdf/build/debug/libjpeg.a mupdf/build/debug/libopenjpeg.a mupdf/build/debug/libz.a -lm -lssl `pkg-config --cflags --libs gobject-2.0` `pkg-config --cflags --libs gtk+-3.0`

# link
bin/juggler: $(OBJS) bin/gui.b
	$(CC) $(LFLAGS) -o bin/juggler $(OBJS) bin/gui.b $(LIBS)

bin/gui.b: src/gui.vala
	valac -g -c --pkg gtk+-3.0 src/gui.vala
	mv gui.vala.o bin/gui.b

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info
bin/%.o: src/%.c
	$(CC) -c $(CFLAGS) src/$*.c -o bin/$*.o
	$(CC) -MM $(CFLAGS) src/$*.c > bin/$*.d
	sed 's_$*.o:_bin/$*.o:_' bin/$*.d > bin/$*.d.new
	mv bin/$*.d.new bin/$*.d

src/rename-lexer.c: src/rename-lexer.l
	flex -o src/rename-lexer.c src/rename-lexer.l

# remove compilation products
clean:
	rm -f bin/juggler bin/*.o bin/*.d