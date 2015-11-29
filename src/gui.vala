/*
  Copyright (C) 2015 Stefan Klein

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.
   
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details. 

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
using Gtk;
using Cairo;

public struct MetaData {
	public string title;
	public string author;
	public string subject;
	public string keywords;
	public string creationDate;
	public string modifyDate;
	public string creator;
	public string producer;
}

public struct RenderData {
	public void *samples; // out
	public void *pixmap; // out -> unused here
	public double zoom; // in
	public int pagenum; // in
	public int x; // in/out
	public int y; // in/out
	public int width; // in/out
	public int height; // in/out
}

public struct PageSize {
	public int x;
	public int width;
	public int height;
}

public struct JugglerCDoc {
	public void *pdf_doc;
	public int pageCount;
	public PageSize *pageSizes;
	public RenderData rendered[5];
}

public enum JugglerErrorCode { NoError, ErrorUsage, ErrorNewContext, ErrorPasswordProtected,
			   ErrorTrailerNoDict, ErrorCatalogNoDict, ErrorCacheObject,
			   ErrorEntryNoObject, ErrorNoInfo, NoMemoryError, 
			   NoDfocumentInfoExists, ErrorNoRoot, ErrorNoPages, 
			   ErrorInvalidRange }


extern int juggler_init(void **init_data);
extern int juggler_open(void *init_data, string filename, JugglerCDoc **document);
extern int juggler_save(JugglerCDoc *document, string filename);
extern int juggler_close(JugglerCDoc *document);
extern int render_page(JugglerCDoc *document, RenderData *data);
extern void free_rendered_data(JugglerCDoc *document, RenderData *data);

extern int juggler_get_info_obj_num(JugglerCDoc *juggler, int *num, int *gen);
extern int juggler_get_root_obj_num(JugglerCDoc *juggler, int *num, int *gen);

extern int juggler_get_meta_data(JugglerCDoc *document, MetaData *data);
extern int juggler_set_meta_data(JugglerCDoc *document, MetaData *data);

extern JugglerErrorCode juggler_remove_page(JugglerCDoc *juggler, int pagenum);
extern JugglerErrorCode juggler_remove_pages(JugglerCDoc *juggler, 
										int firstIndex, int lastIndex);
extern int juggler_add_pages_from_file(JugglerCDoc *dest, JugglerCDoc *src, int posIndex);

extern JugglerErrorCode juggler_get_page_rotation(JugglerCDoc *juggler, int index, int *rotation);
extern JugglerErrorCode juggler_set_page_rotation(JugglerCDoc *juggler, int index, int rotation);

extern JugglerErrorCode juggler_put_page_contents(JugglerCDoc *juggler, string filename);

extern JugglerErrorCode juggler_export_images(JugglerCDoc *juggler, string path);


public enum DumpType { NoFormat, Reference, Stream }

public struct JugglerDump {
	public JugglerDump *next;
	public DumpType type;
	public size_t textLen;
	public char text;	
}

extern JugglerErrorCode juggler_dump_object(JugglerCDoc *juggler, int num, int gen, JugglerDump **dump_out);
extern int juggler_dump_delete(JugglerDump *dump);



int main (string[] args) {
    Gtk.init (ref args);

    var window = new Juggler();
    window.show_all();

    Gtk.main();
    return 0;
}

public class Juggler : Window {
	Toolbar toolbar;
	ToolButton openButton;
	ToolButton tryButton;
	ToolItem pageEntryItem;
	Entry pageEntry;
	JugglerView view;
	JugglerDocument doc;
	Box mainBox;
	Box viewBox;
	Scrollbar scrollbar;
	Scrollbar hScrollbar;
	ScrolledWindow scrolledWindow;
	MenuBar menuBar;
	Gtk.MenuItem fileMenuItem;
	Gtk.Menu fileMenu;
	Gtk.MenuItem fileOpen;
	Gtk.MenuItem fileSave;
	Gtk.MenuItem fileExportImages;
	Gtk.MenuItem fileProperties;
	Gtk.MenuItem fileQuit;
	Gtk.MenuItem documentMenuItem;
	Gtk.Menu documentMenu;
	Gtk.MenuItem documentStructure;
	Gtk.MenuItem documentRemovePages;
	Gtk.MenuItem documentAddPages;
	Gtk.MenuItem documentRotatePages;
	Gtk.MenuItem documentPutPageContents;

	void *initData;

	public Juggler() {
		this.title = "Juggler";
		this.window_position = WindowPosition.CENTER;
		set_default_size(800, 600);

		menuBar = new MenuBar();

		fileMenuItem = new Gtk.MenuItem.with_label("File");
		menuBar.add(fileMenuItem);

		fileMenu = new Gtk.Menu();
		fileMenuItem.set_submenu(fileMenu);
		
		fileOpen = new Gtk.MenuItem.with_label("Open");
		fileOpen.activate.connect(OnOpen);
		fileMenu.add(fileOpen);

		fileSave = new Gtk.MenuItem.with_label("Save");
		fileSave.activate.connect(OnSave);
		fileMenu.add(fileSave);
		
		fileExportImages = new Gtk.MenuItem.with_label("Export images");
		fileExportImages.activate.connect(OnFileExportImages);
		fileMenu.add(fileExportImages);

		fileProperties = new Gtk.MenuItem.with_label("Properties");
		fileProperties.activate.connect(OnFileProperties);
		fileMenu.add(fileProperties);

		fileQuit = new Gtk.MenuItem.with_label("Quit");
		fileQuit.activate.connect(OnFileQuit);
		fileMenu.add(fileQuit);

		documentMenuItem = new Gtk.MenuItem.with_label("Document");
		menuBar.add(documentMenuItem);

		documentMenu = new Gtk.Menu();
		documentMenuItem.set_submenu(documentMenu);

		documentStructure = new Gtk.MenuItem.with_label("Structure");
		documentStructure.activate.connect(OnDocumentStructure);
		documentMenu.add(documentStructure);


		documentRemovePages = new Gtk.MenuItem.with_label("Remove Pages");
		documentRemovePages.activate.connect(OnDocumentRemovePages);
		documentMenu.add(documentRemovePages);

		documentAddPages = new Gtk.MenuItem.with_label("Add Pages");
		documentAddPages.activate.connect(OnDocumentAddPages);
		documentMenu.add(documentAddPages);

		documentRotatePages = new Gtk.MenuItem.with_label("Rotate Pages");
		documentRotatePages.activate.connect(OnDocumentRotatePages);
		documentMenu.add(documentRotatePages);


		documentPutPageContents = new Gtk.MenuItem.with_label("Impose two-on-one");
		documentPutPageContents.activate.connect(OnDocumentPutPageContents);
		documentMenu.add(documentPutPageContents);

		toolbar = new Toolbar();
		toolbar.get_style_context().add_class(STYLE_CLASS_PRIMARY_TOOLBAR);

		openButton = new ToolButton.from_stock(Stock.OPEN);
		openButton.is_important = true;
		openButton.clicked.connect(OnOpen);
		toolbar.add(openButton);

		tryButton = new ToolButton.from_stock(Stock.PREFERENCES);//(null, "Test");
//		tryButton.clicked.connect(OnFileProperties);
		tryButton.clicked.connect(OnDocumentStructure);
		toolbar.add(tryButton);

		pageEntryItem = new ToolItem();
		pageEntry = new Entry();
		pageEntryItem.add(pageEntry);
		toolbar.insert(pageEntryItem, -1);
		pageEntry.activate.connect(OnPageEntryActivate);
add_events(Gdk.EventMask.SCROLL_MASK);

		juggler_init(&initData);
//		doc = new JugglerDocument(initData, "samples/ExampleLayout.pdf");
//		doc = new JugglerDocument(initData, "samples/MitBild.pdf");

		view = new JugglerView();
		doc = null;
		view.SetDocument(doc);
		view.PageChanged.connect(OnPageChanged);

/*		scrolledWindow = new ScrolledWindow(null, null);
		scrolledWindow.set_hexpand(true);
		scrolledWindow.set_vexpand(true);
		scrolledWindow.set_size_request(500, 500);
		scrolledWindow.add(view);*/
		
		viewBox = new Box(Orientation.HORIZONTAL, 0);
		viewBox.pack_start(view, true, true, 0);

		scrollbar = new Scrollbar(Orientation.VERTICAL, null);
		view.vadjustment = scrollbar.get_adjustment();
		viewBox.pack_start(scrollbar, false, false, 0);

		hScrollbar = new Scrollbar(Orientation.HORIZONTAL, null);
		view.hadjustment = hScrollbar.get_adjustment();

		mainBox = new Box(Orientation.VERTICAL, 0);
		mainBox.pack_start(menuBar, false, true, 0);
		mainBox.pack_start(toolbar, false, true, 0);
		mainBox.pack_start(viewBox, true, true, 0);
		//mainBox.pack_start(scrolledWindow, true, true, 0);
		mainBox.pack_start(hScrollbar, false, false, 0);

		add(mainBox);

		destroy.connect(Gtk.main_quit);
	}

	void OnOpen() {
		string filename = StockDialogs.Open(this);
		if(filename == null)
			return;

		doc = new JugglerDocument(initData, filename);
		view.SetDocument(doc);
	}

	void OnSave() {
		if(doc == null)
			return;

		string filename = StockDialogs.Save(this);
		if(filename == null)
			return;
		
		doc.Save(filename);
	}
	
	void OnFileExportImages() {
		if(doc == null)
			return;
		
		string path = StockDialogs.SelectFolder(this);
		if(path == null)
			return;
		
		doc.ExportImages(path + GLib.Path.DIR_SEPARATOR_S);
	}

	void OnPageEntryActivate() {
		view.currentPage = int.parse(pageEntry.text) - 1;
	}

	void OnPageChanged(int pageIndex) {
		pageEntry.text = (pageIndex + 1).to_string(); // TODO: print page-name
	}

	void OnFileProperties() {
		if(doc == null)
			return;

		FilePropertiesDlg dlg = new FilePropertiesDlg(doc, this);
		dlg.Run();
	}

	void OnFileQuit() {
		destroy();
	}

	void OnDocumentStructure() {
		if(doc == null)
			return;

		DocumentStructureDlg dlg = new DocumentStructureDlg(this, doc);
		dlg.Run();
	}

	void OnDocumentRemovePages() {
		if(doc == null)
			return;

		SelectPagesDlg dlg = new SelectPagesDlg(this, doc.pageCount);
		dlg.firstIndex = view.currentPage;
		dlg.lastIndex = view.currentPage;
		if(!dlg.Run())
			return;

		if(!StockDialogs.YesNo(this, "Do you really delete the contents of page " + 
			(dlg.firstIndex + 1).to_string() + " to " + (dlg.lastIndex + 1).to_string()))
		{
			return;
		}

		try {
			doc.RemovePages(dlg.firstIndex, dlg.lastIndex);
		} catch(JugglerError e) {
			// show error dialog
			StockDialogs.Error(this, e.message);
			return;
		}
	}

	void OnDocumentAddPages() {
		if(doc == null)
			return;

		string filename = StockDialogs.Open(this);

		if(filename == null)
			return;

		InsertPagesDlg dlg = new InsertPagesDlg(this);
		dlg.pageIndex = view.currentPage;
		dlg.pageCount = doc.pageCount;

		if(!dlg.Run())
			return;

		doc.AddPages(filename, dlg.pageIndex);
	}

	void OnDocumentRotatePages() {
		if(doc == null)
			return;

		RotatePagesDlg dlg = new RotatePagesDlg(this);
		dlg.pageCount = doc.pageCount;
		dlg.fromIndex = dlg.toIndex = view.currentPage;

		if(!dlg.Run())
			return;

		for(int i = dlg.fromIndex; i <= dlg.toIndex; i++) {
			int newRotation = (doc.GetPageRotation(i) + dlg.degrees) % 360;
			doc.SetPageRotation(i, newRotation);
		}
	}

	void OnDocumentPutPageContents() {
		if(doc == null)
			return;

		string filename = StockDialogs.Save(this);
		if(filename == null)
			return;
			
		doc.PutPageContents(filename);
	}
}

public errordomain JugglerError {
	InvalidRange, UnknownError
}

public class JugglerDocument {
	JugglerCDoc *juggler;
	void *initData;
	RenderData *lastRendered;

	public int pageCount {
		get { return(juggler->pageCount); }
	}

	public string path {
		public get; private set;
	}

	public signal void DocumentChanged();

	public JugglerDocument(void *initData, string filename) {
		this.initData = initData;
		lastRendered = null;

		juggler_open(initData, filename, &juggler);

		stdout.printf("Vala Code: I know the pagenum: %d\n", juggler->pageCount);

		for(int i = 0; i < juggler->pageCount; i++) {
			stdout.printf("[V] Display Box of page %d [ %d %d %d ]\n", i + 1, 
						  juggler->pageSizes[i].x, 
						  juggler->pageSizes[i].width, 
						  juggler->pageSizes[i].height);
		}

		path = filename;
	}

	~JugglerDocument() {
		juggler_close(juggler);
	}

	void HandleErrors(JugglerErrorCode error) throws JugglerError {
			switch(error) {
			case JugglerErrorCode.NoError:
				break;
			case JugglerErrorCode.ErrorInvalidRange:
				throw new JugglerError.InvalidRange("Invalid page range");
			default:
				throw new JugglerError.UnknownError("I can't tell what, but something went wrong!");
			}
	}

	public bool Save(string filename) {
		return(juggler_save(juggler, filename) == 0);
	}

	public void Render(RenderData *data) {
		render_page(juggler, data);
	}

	public void FreeRenderData(RenderData *data) {
		free_rendered_data(juggler, data);
	}

	public int GetPageX(int pagenum) {
		if(pagenum < 0 || pagenum > pageCount)
			return(-1);
		return(juggler->pageSizes[pagenum].x);
	}


	public int GetPageHeight(int pagenum) {
		if(pagenum < 0 || pagenum > pageCount)
			return(-1);
		return(juggler->pageSizes[pagenum].height);
	}

	public int GetPageWidth(int pagenum) {
		if(pagenum < 0 || pagenum > pageCount)
			return(-1);
		return(juggler->pageSizes[pagenum].width);
	}

	public void GetRootNum(out int num, out int gen) {
		num = gen = 0; // prevents us from a warning
		juggler_get_root_obj_num(juggler, &num, &gen);
	}

	public void GetInfoNum(out int num, out int gen) {
		num = gen = 0; // prevents us from a warning
		juggler_get_info_obj_num(juggler, &num, &gen);
	}

	public MetaData GetMetaData() {
		MetaData data = MetaData();
		juggler_get_meta_data(juggler, &data);

		return(data);
	}

	public void SetMetaData(MetaData data) {
		juggler_set_meta_data(juggler, &data);
	}

	public JugglerDump *DumpObject(int num, int gen) throws JugglerError {
		JugglerDump *dump = null;
		HandleErrors(juggler_dump_object(juggler, num, gen, &dump));

		return(dump);
	}

	public void FreeDump(JugglerDump *dump) {
		juggler_dump_delete(dump);
	}

	public void RemovePage(int pagenum) throws JugglerError {
		HandleErrors(juggler_remove_page(juggler, pagenum));
		DocumentChanged();
	}

	public void RemovePages(int firstIndex, int lastIndex) 
		throws JugglerError
	{
		HandleErrors(juggler_remove_pages(juggler, firstIndex, lastIndex));
		DocumentChanged();
	}

	public void AddPages(string srcFile, int posIndex) throws JugglerError {
		JugglerDocument doc = new JugglerDocument(initData, srcFile);
		juggler_add_pages_from_file(juggler, doc.juggler, posIndex);

		DocumentChanged();
	}

	public int GetPageRotation(int index) {
		int rotation = 0;
		HandleErrors(juggler_get_page_rotation(juggler, index, &rotation));

		return(rotation);
	}

	public void SetPageRotation(int index, int rotation) {
		HandleErrors(juggler_set_page_rotation(juggler, index, rotation));
		DocumentChanged();
	}

	public void PutPageContents(string filename) {
		juggler_put_page_contents(juggler, filename);
		//DocumentChanged();
	}

	public void ExportImages(string path) { // needs to have a trailing '/'
		juggler_export_images(juggler, path);
		DocumentChanged();
	}
}

public enum ViewLayout { SinglePage, DoublePage }

private struct LayoutedPage {
	public double x;
	public double y;
	public double width;
	public double height;
}

public class JugglerView : DrawingArea, Scrollable {
	const int PagePadding = 10; // padding between two subsequent pages
	const int PerScrollMove = 25; // how much to move on mouse-scrolling

	JugglerDocument doc;

	double zoom;

	private int currentPageData;
	public int currentPage { 
		get {
			return(currentPageData);
		} 
		set {
			if(value < 0 || value >= doc.pageCount)
				return;

			vadjustmentData.value = layoutedPages.index(value).y;

			queue_draw();
		}
	}

	private Array<LayoutedPage?> layoutedPages;
	private bool needsRelayout;

	private Allocation currentAllocation;

	private Adjustment hadjustmentData;
	private Adjustment vadjustmentData;
	public Adjustment hadjustment {
		get {
			return(hadjustmentData);
		}
		set {
			hadjustmentData = value;
			value.lower = 0;
			value.upper = 0;
			value.value = 0;
			value.step_increment = 1;
			value.page_increment = PerScrollMove;
			value.page_size = 0;
			value.value_changed.connect(HAdjustmentChanged);
		}
	}
	public Adjustment vadjustment {
		get {
			return(vadjustmentData);
		}
		set { 
			vadjustmentData = value; 
			value.value = 0;
			value.lower = 0;
			value.upper = 1200;
			value.step_increment = 1; 
			value.page_increment = PerScrollMove; 
			value.page_size = 1;
			value.value_changed.connect(VAdjustmentChanged); 
		}
	}
	public ScrollablePolicy hscroll_policy { get; set; }
	public ScrollablePolicy vscroll_policy { get; set; }

	public double scrollPosY { get { return(vadjustmentData.get_value()); } }


	public signal void PageChanged(int newPageIndex);
	public signal void ZoomChanged(int zoomPercentage);

	const int Padding = 10; // Padding between pages

	int allocX;
	int allocY;
	int allocWidth;
	int allocHeight;

	public JugglerView() {
		doc = null;
		zoom = 1.0;

		can_focus = true;
		add_events(Gdk.EventMask.SCROLL_MASK | Gdk.EventMask.KEY_RELEASE_MASK);

		vadjustmentData = null;
		vscroll_policy = ScrollablePolicy.MINIMUM;
		hscroll_policy = ScrollablePolicy.MINIMUM;
		hadjustmentData = null;

		layoutedPages = new Array<LayoutedPage?>(); // TODO: Create this array item's once for every document, not on every zoom/...
		needsRelayout = true;

		
		
		draw.connect(OnDraw);
		scroll_event.connect(OnScroll);
		size_allocate.connect(OnResize);
		key_release_event.connect(OnKeyUp);
	}

	public void VAdjustmentChanged() {
		// find active page
		// TODO: Make this a bit better... but what is the best way???
		for(int i = 0; i < doc.pageCount; i++) {
			if(layoutedPages.index(i).y + layoutedPages.index(i).height > vadjustment.value) {
				currentPageData = i;
				PageChanged(i);
				break;
			}
		}

		queue_draw();
	}

	public void HAdjustmentChanged() {
		queue_draw();
	}

	// what document should be shown by this view? may also be set to null!
	public void SetDocument(JugglerDocument newDoc) {
		doc.DocumentChanged.disconnect(OnDocumentChanged);
		doc = newDoc;
		newDoc.DocumentChanged.connect(OnDocumentChanged);

		zoom = 1.0;
		LayoutPages();
		currentPage = 0;

		queue_draw();
	}

	void OnDocumentChanged() {
		// TODO: Call LayoutPages()
		needsRelayout = true;
		queue_draw();
	}

    bool OnDraw(Context context) {
		if(doc == null) {
			//context.rectangle(20, 20, 100, 100);
			//context.set_source_rgb(1, 0, 0);
			//context.fill();
		} else {
			LayoutPages();

			double visibleHeight = get_allocated_height();
			double visibleWidth = get_allocated_width();
			double minX = 0;
			double maxX = 0;

			// we just use ints to prevent aliasing
			if((hadjustment.upper - hadjustment.lower) <= hadjustment.page_size)
				context.translate((int) visibleWidth / 2, (int) (- scrollPosY));
			else
				context.translate((int) (- hadjustment.value), (int) (- scrollPosY));
			

			context.set_source_rgba(0.2, 0.2, 0.2, 1.0); // make a nice background
			context.paint();

			for(int i = 0; i < doc.pageCount; i++) {
				if(layoutedPages.index(i).y + layoutedPages.index(i).height < scrollPosY)
					continue;
				if(layoutedPages.index(i).y > scrollPosY + visibleHeight)
				  break;

				if(layoutedPages.index(i).x - Padding < minX)
					minX = layoutedPages.index(i).x - Padding;
				if(layoutedPages.index(i).x + layoutedPages.index(i).width + Padding * 2 > maxX)
					maxX = layoutedPages.index(i).x + layoutedPages.index(i).width + Padding;

				stdout.printf("Drawing page %d\n", i);

				RenderData data = RenderData();
				data.pagenum = i;
				data.zoom = zoom;
				data.x = 0;
				data.y = 0;
				data.width = 1;
				data.height = 1;
				doc.Render(&data);

				context.set_antialias(Antialias.NONE);
				ImageSurface surface = new ImageSurface.for_data(
					(uchar []) data.samples, Cairo.Format.ARGB32, 
					data.width, data.height, data.width * 4);

				context.set_source_surface(surface, (int) layoutedPages.index(i).x, 
										   (int) layoutedPages.index(i).y);
				context.paint();

				doc.FreeRenderData(&data);
			}


			if(hadjustmentData != null) {

				hadjustmentData.page_size = visibleWidth;
				hadjustmentData.lower = minX;
				hadjustmentData.upper = maxX;

				if(hadjustmentData.value > hadjustmentData.upper - hadjustmentData.page_size)
					hadjustmentData.value = hadjustmentData.upper - hadjustmentData.page_size;
				if(hadjustmentData.value < hadjustmentData.lower)
					hadjustmentData.value = hadjustmentData.lower;
			}

		}
		return true;
    }

	public bool OnScroll(Gdk.EventScroll event) {
		if((event.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
			if(event.direction == Gdk.ScrollDirection.SMOOTH)
				Zoom(event.delta_x > 0, event.x, event.y);
			else
				Zoom(event.direction == Gdk.ScrollDirection.UP, event.x, event.y);
		} else {
			if(event.direction == Gdk.ScrollDirection.SMOOTH)
				ScrollY((int) (PerScrollMove * event.delta_x));
			else
				ScrollY(event.direction == Gdk.ScrollDirection.UP ? 
				- PerScrollMove : PerScrollMove);
		}	


		return(true);
	}

	public void OnResize(Allocation allocation) {
		stdout.printf("ALLOCATION-RECT: %d %d %d %d\n", allocation.x, allocation.y, allocation.width, allocation.height);
		// TODO: Layout Pages???
		allocX = allocation.x;
		allocY = allocation.y;
		allocWidth = allocation.width;
		allocHeight = allocation.height;
		needsRelayout = true;
	}

	public void LayoutPages() {
		ViewLayout layout = ViewLayout.SinglePage;

		layoutedPages.remove_range(0, layoutedPages.length);

		double currentY = Padding;
		if(layout == ViewLayout.SinglePage) {
			for(int i = 0; i < doc.pageCount; i++) {
				LayoutedPage page = LayoutedPage();
				page.width = doc.GetPageWidth(i) * zoom;
				page.x = - page.width / 2;
				page.height = doc.GetPageHeight(i) * zoom;
				page.y = currentY;
				layoutedPages.append_val(page);

				currentY += page.height + Padding; // don't zoom Padding
			}
		}
		if(vadjustmentData != null) {
			vadjustmentData.page_size = get_allocated_height();
			vadjustmentData.upper = currentY;

			if(vadjustmentData.value > vadjustmentData.upper - vadjustmentData.page_size)
				vadjustmentData.value = vadjustmentData.upper - vadjustmentData.page_size;
		}

		//	height_request = currentY;
		//width_request = 1000;

		needsRelayout = false;
	}

	public void ScrollY(int deltaY) {
		vadjustmentData.value += deltaY;

		queue_draw();
	}

	public void SetZoom(int zoomPercentage) {
		zoom = (double) zoomPercentage / 100.0;
	}

	public void Zoom(bool zoomIn, double preserveX, double preserveY) {
		const size_t ZoomValuesCount = 17;
		const double ZoomValues[] = { 0.125, 0.250, 0.33, 0.50, 0.75, 0.90, 1.00, 
     	    1.10, 1.25, 1.41, 1.50, 1.75, 2.00, 4.00, 6.00, 12.00, 24.00 };

		stdout.printf("Preserve %f | %f\n", preserveX, preserveY);

		if(zoomIn) {
			size_t i;
			for(i = 0; i < ZoomValuesCount; i++) {
				stdout.printf("%.2f - ", ZoomValues[i]);
				if(ZoomValues[i] - zoom > 0.0) {
					zoom = ZoomValues[i];
					break;
				}
			}
		} else {
			size_t i;
			for(i = ZoomValuesCount; i > 0; i--) {
				stdout.printf("%.2f - ", ZoomValues[i - 1]);
				if(zoom - ZoomValues[i - 1] > 0.0) {
					zoom = ZoomValues[i - 1];
					break;
				}
			}
		}

        // It's a bad design, but adjust the h- and v-position according to the 
		vadjustmentData.upper *= zoom;
		vadjustmentData.value *= zoom;
		//hadjustmentData.upper *= zoom;
		//hadjustmentData.value *= zoom;

		

		needsRelayout = true;

		queue_draw();
	}

	public bool OnKeyUp(Gdk.EventKey keyEvent) {
		
		if(keyEvent.keyval == 0xff55) // GDK_KEY_Page_Up
			currentPage--;
		else if(keyEvent.keyval == 0xff56) // GDK_KEY_Page_Down
			currentPage++;
		else if(keyEvent.keyval == 0xff50) // GDK_KEY_Home
			currentPage = 0;
		else if(keyEvent.keyval == 0xff57) // GDK_KEY_End
			currentPage = doc.pageCount - 1;
		else
			return(false);

		return(true);
	}
}

public class FilePropertiesDlg {
	JugglerDocument doc;
	Builder builder;
	Dialog dialog;

	MetaData data;

	Label pathLabel;
	Entry titleEntry;
	Entry authorEntry;
	Entry subjectEntry;
	TextView keywordsTextView;
	Entry creationDateEntry;
	Entry modifyDateEntry;
	Entry applicationEntry;
	
	Entry producerEntry;
	/*Label pdfVersionLabel;
	Label fileSizeLabel;
	Label pageSizeLabel;
	Label taggedPDFLabel;
	Label numberOfPagesLabel;*/


	public FilePropertiesDlg(JugglerDocument doc, Juggler window) {
		builder = new Builder();
		try {
			builder.add_from_file("ui/fileProperties.glade");
		} catch(Error e) {
			return; // TODO: Break whole program in this case
		}
		builder.connect_signals(null);
		dialog = builder.get_object("fileProperties") as Dialog;
		
		this.doc = doc;
		data = doc.GetMetaData();

		pathLabel = builder.get_object("pathLabel") as Label;
		pathLabel.label = doc.path;
		titleEntry = builder.get_object("titleEntry") as Entry;
		titleEntry.text = data.title != null ? data.title : "";
		authorEntry = builder.get_object("authorEntry") as Entry;
		authorEntry.text = data.author != null ? data.author : "";
		subjectEntry = builder.get_object("subjectEntry") as Entry;
		subjectEntry.text = data.subject != null ? data.subject : "";
		keywordsTextView = builder.get_object("keywordsTextView") as TextView;
		keywordsTextView.buffer.text = data.keywords != null ? data.keywords : "";
		creationDateEntry = builder.get_object("creationDateEntry") as Entry;
		creationDateEntry.text = data.creationDate != null ? data.creationDate : "";
		modifyDateEntry = builder.get_object("modifyDateEntry") as Entry;
		modifyDateEntry.text = data.modifyDate != null ? data.modifyDate : "";
		applicationEntry = builder.get_object("applicationEntry") as Entry;
		applicationEntry.text = data.creator != null ? data.creator : "";
	
		producerEntry = builder.get_object("producerEntry") as Entry;
		producerEntry.text = data.producer != null ? data.producer : "";
		//pdfVersionLabel = builder.get_object("pdfVersionLabel") as Label; 
		//pdfVersionLabel.label = "PDF " + (data.pdfVersion / 10).to_string() + "." + (data.pdfVersion % 10).to_string();
		//fileSizeLabel = builder.get_object("fileSizeLabel") as Label;
		//fileSizeLabel.label = data.fileSize;
//		pageSizeLabel = builder.get_object("pageSizeLabel") as Label; pageSizeLabel.label = data.pageSize;
//		taggedPDFLabel = builder.get_object("taggedPDFLabel") as Label;
//		numberOfPagesLabel = builder.get_object("numberOfPagesLabel") as Label; agesLabel.label = data.ages

//		Button okButton = builder.get_object("okButton") as Button;
		//okButton.clicked.connect(OnOK);

	}

	public void Run() {
		if(dialog.run() == ResponseType.OK) {
			data.title = titleEntry.text_length > 0 ? titleEntry.text : null;
			data.author = authorEntry.text_length > 0 ? authorEntry.text : null;
			data.subject = subjectEntry.text_length > 0 ? subjectEntry.text : null;
			data.keywords = keywordsTextView.buffer.text.length > 0 ? keywordsTextView.buffer.text : null;
			data.creationDate = creationDateEntry.text_length > 0 ? creationDateEntry.text : null;
			data.modifyDate = modifyDateEntry.text_length > 0 ? modifyDateEntry.text : null;
			data.creator = applicationEntry.text_length > 0 ? applicationEntry.text : null;
			data.producer = producerEntry.text_length > 0 ? producerEntry.text : null;
			doc.SetMetaData(data);
		}
		dialog.destroy();
		dialog = null;
	}
/*
	void OnOK() {
		stdout.printf("+++++++++++++++++++++++++++++++++++ OK +++++++++++\n");
		dialog.response(ResponseType.OK);
	}

	void OnCancel() {
		dialog.response(ResponseType.CANCEL);
	}
*/
}


public class DocumentStructureDlg {
	JugglerDocument doc;
	Builder builder;
	Dialog dialog;
	Button backButton;
	Entry currentEntry;
	TextView objectView;
	TextTag linkTag;
	TreeView historyView;
	Gtk.ListStore historyListStore;
	JugglerDump *currentDump;
	int currentNum;
	int currentGen;
	TreeIter currentHistoryPosition;
	bool currentHistoryPositionInitialized; // is set false in the constructor

	const int MaxBinStreamSize = 1024 * 50;
	
	public DocumentStructureDlg(Juggler window, JugglerDocument doc) {
		builder = new Builder();
		try {
		builder.add_from_file("ui/documentStructure.glade");
		} catch(Error e) {
			return; // TODO: Quit everything in that case
		}
		builder.connect_signals(null);
		dialog = builder.get_object("documentStructure") as Dialog;

		backButton = builder.get_object("backButton") as Button;
		backButton.clicked.connect(OnBack);
		currentEntry = builder.get_object("currentEntry") as Entry;
		currentEntry.activate.connect(OnGoTo);

		historyView = builder.get_object("historyView") as TreeView;
		historyView.row_activated.connect(OnHistoryDoubleClick);
		historyListStore = builder.get_object("historyListStore") as Gtk.ListStore;
		currentHistoryPositionInitialized = false;
		CellRendererText cellRenderer = builder.get_object("historyCellRenderer") as CellRendererText;
		cellRenderer.height = 72;

		objectView = builder.get_object("objectView") as TextView;
		objectView.override_font(Pango.FontDescription.from_string("Monospace 12"));

		currentDump = null;

		this.doc = doc;
	}

	public void Run() {
		TextTagTable tags = objectView.buffer.get_tag_table();
		
		linkTag = new TextTag();
		linkTag.underline = Pango.Underline.SINGLE;
		linkTag.foreground = "blue";
		linkTag.event.connect(OnLinkClicked);
		tags.add(linkTag);

		LoadObjectDump(1, 0);
		AddCurrentToHistory();

		dialog.run();
		dialog.destroy();
		dialog = null;
	}

	void PutStream(char *content, int length) {
		TextIter endIter;
		objectView.buffer.get_end_iter(out endIter);

		const string StreamHeader = "\n\nStream:\n";
		objectView.buffer.insert(ref endIter, StreamHeader, StreamHeader.length);

		objectView.buffer.get_end_iter(out endIter);

		bool isBinary = false;
		for(int i = 0; i < length && i < MaxBinStreamSize; i++) {
			if((*(content + i) < 0x20 || *(content + i) > 0x7E) && 
			   !(*(content + i) == '\t' || *(content + i) == '\n' || *(content + i) == '\r')) {
				isBinary = true;
				break;
			}
		}

		if(!isBinary) {
			stdout.printf("Doing dumping thing\n");
			objectView.buffer.insert(ref endIter, (string) content, length);
		} else {
			string hexOutput = "";
			for(int i = 0; i + 7 < length && i < MaxBinStreamSize; i += 8) {
				hexOutput += "%.8x: %.2x %.2x %.2x %.2x  %.2x %.2x %.2x %.2x | %c%c%c%c%c%c%c%c\n".printf(i,
				  *(content + i + 0) & 0xFF, *(content + i + 1) & 0xFF, 
				  *(content + i + 2) & 0xFF, *(content + i + 3) & 0xFF,
				  *(content + i + 4) & 0xFF, *(content + i + 5) & 0xFF, 
				  *(content + i + 6) & 0xFF, *(content + i + 7) & 0xFF,
				  *(content + i + 0) < 0x20 || *(content + i + 0) > 0x7E ? '.' : *(content + i + 0),
                  *(content + i + 1) < 0x20 || *(content + i + 1) > 0x7E ? '.' : *(content + i + 1),
                  *(content + i + 2) < 0x20 || *(content + i + 2) > 0x7E ? '.' : *(content + i + 2), 
                  *(content + i + 3) < 0x20 || *(content + i + 3) > 0x7E ? '.' : *(content + i + 3), 
                  *(content + i + 4) < 0x20 || *(content + i + 4) > 0x7E ? '.' : *(content + i + 4),
                  *(content + i + 5) < 0x20 || *(content + i + 5) > 0x7E ? '.' : *(content + i + 5),
                  *(content + i + 6) < 0x20 || *(content + i + 6) > 0x7E ? '.' : *(content + i + 6),
                  *(content + i + 7) < 0x20 || *(content + i + 7) > 0x7E ? '.' : *(content + i + 7));
			}
			objectView.buffer.insert(ref endIter, hexOutput, hexOutput.length);
		}
	}

	void AddCurrentToHistory() {
		char *dumpContentPtr = &currentDump->text;
		unowned string dumpContent = (string) dumpContentPtr;
		
		long startIndex = (dumpContent.substring(0, 3) == "<<\n") ? 3 : 0; // skip the dictionary-begin
		long endIndex = startIndex;
		for(int i = 0; endIndex < currentDump->textLen && i < 2; endIndex++)
			if(dumpContent[endIndex] == '\n')
				i++;
		endIndex--; // else we would print the last '\n'
		string dumpExcerpt = dumpContent.substring(startIndex, endIndex - startIndex);
		dumpExcerpt = dumpExcerpt.replace("<", "&lt;");
		dumpExcerpt = dumpExcerpt.replace(">", "&gt;");
		string historyText = 
			"<b>" + currentNum.to_string() + " " + currentGen.to_string() + " obj</b>\n" + 
			dumpExcerpt;

		TreeIter listIter;
		if(!currentHistoryPositionInitialized)
			historyListStore.insert_after(out listIter, null);
		else
			historyListStore.insert_after(out listIter, currentHistoryPosition);
		historyListStore.set(listIter, 0, historyText, 1, currentNum, 2, currentGen, -1);

		currentHistoryPosition = listIter;
		currentHistoryPositionInitialized = true;


		/* delete all history-entries after the new added */
		while(historyListStore.iter_next(ref listIter)) {
			historyListStore.remove(listIter);
			listIter = currentHistoryPosition;
		}
	}

	void LoadObjectDump(int num, int gen) {
		TextIter endIter;

		objectView.buffer.text = "";

		if(currentDump != null)
			doc.FreeDump(currentDump);

		JugglerDump *dump = doc.DumpObject(num, gen);
		currentDump = dump;
		while(dump != null) {
			char *text = &dump->text;

			objectView.buffer.get_end_iter(out endIter);

			if(dump->type == DumpType.NoFormat)
				objectView.buffer.insert(ref endIter, (string) text, (int) dump->textLen);
			else if(dump->type == DumpType.Reference)
				objectView.buffer.insert_with_tags(ref endIter, (string) text, (int) dump->textLen, linkTag, null);
			else if(dump->type == DumpType.Stream)
				PutStream((char *) text, (int) dump->textLen);
				

			dump = dump->next;
		}

		currentNum = num;
		currentGen = gen;
		currentEntry.text = currentNum.to_string() + " " + currentGen.to_string();
	}

	bool OnLinkClicked(Object eventObject, Gdk.Event event, TextIter iter) {
		if(event.type == Gdk.EventType.BUTTON_RELEASE) {
			TextIter tagStart = iter;
			tagStart.backward_to_tag_toggle(linkTag);
			TextIter tagEnd = iter;
			tagEnd.forward_to_tag_toggle(linkTag);
			
			string linkText = objectView.buffer.get_text(tagStart, tagEnd, true);
			int num = 1;
			int gen = 0;
			linkText.scanf("%d %d R", &num, &gen);

			LoadObjectDump(num, gen);
			AddCurrentToHistory();
			historyView.get_selection().select_iter(currentHistoryPosition);


			return(false);
		}

		return(false);
	}

	void OnBack() {
		TreeIter previousIter = currentHistoryPosition;
		// don't do anything else if we reached the first item
		if(!historyListStore.iter_previous(ref previousIter))
			return;

		int num = 0;
		int gen = 0;
		historyListStore.get(previousIter, 1, &num, 2, &gen);

		currentHistoryPosition = previousIter;
		historyView.get_selection().select_iter(currentHistoryPosition);
		LoadObjectDump(num, gen);
	}

	void OnGoTo() {
		int num = 1;
		int gen = 0;
		if(Regex.match_simple("^\\d+ \\d+( R)?$", currentEntry.text))
			currentEntry.text.scanf("%d %d R", &num, &gen);
		else if(currentEntry.text == "Info")
			doc.GetInfoNum(out num, out gen);
		else if(currentEntry.text == "Root")
			doc.GetRootNum(out num, out gen);
		else
			return; // invalid format

			LoadObjectDump(num, gen);
			AddCurrentToHistory();
			historyView.get_selection().select_iter(currentHistoryPosition);		
	}

	void OnHistoryDoubleClick(TreePath path, TreeViewColumn column) {
		TreeIter selected;
		historyView.get_selection().get_selected(null, out selected); // may this be null?
		currentHistoryPosition = selected;

		int num = 0;
		int gen = 0;
		historyListStore.get(selected, 1, &num, 2, &gen);
		LoadObjectDump(num, gen);
	}
	
}

public class SelectPagesDlg {
	Builder builder;
	Dialog dialog;
	Entry fromEntry;
	Entry toEntry;
	int pageCount;

	public int firstIndex { get; set; }
	public int lastIndex { get; set; }

	public SelectPagesDlg(Juggler window, int pageCount) {
		builder = new Builder();
		try {
			builder.add_from_file("ui/selectPages.glade");
		} catch(Error e) {
			dialog = null;
			return; // Run() won't work if dialog == null
		}
		dialog = builder.get_object("selectPages") as Dialog;

		fromEntry = builder.get_object("fromEntry") as Entry;
		toEntry = builder.get_object("toEntry") as Entry;
		Label ofLabel = builder.get_object("ofLabel") as Label;
		ofLabel.label = "of " + pageCount.to_string();

		this.pageCount = pageCount;
	}

	public bool Run() {
		if(dialog == null)
			return(false);

		fromEntry.text = (firstIndex + 1).to_string();
		toEntry.text = (lastIndex + 1).to_string();

		ResponseType response;
		while((response = (ResponseType) dialog.run()) == ResponseType.OK) {
			firstIndex = int.parse(fromEntry.text) - 1;
			lastIndex = int.parse(toEntry.text) - 1;

			if(firstIndex >= 0 && firstIndex < pageCount &&
			   lastIndex >= 0 && lastIndex < pageCount &&
			   lastIndex >= firstIndex &&
			   !(firstIndex == 0 && lastIndex == pageCount - 1))
			{
				break;
			}
		}

		dialog.destroy();
		return(response == ResponseType.OK);
	}
}

public class InsertPagesDlg {
	Builder builder;
	Dialog dialog;
	Entry pageNumberEntry;
	Label ofLabel;
	RadioButton locationAfterRadio;
	RadioButton pageFirstRadio;
	RadioButton pageLastRadio;

	int lastPageIndex;
	int selectedIndex;

	public int pageIndex { 
		get {
			return(selectedIndex);
		}
		set {
			pageNumberEntry.text = (value + 1).to_string();
		}
	}
	public int pageCount { 
		set {
			ofLabel.label = "of " + value.to_string();
			lastPageIndex = value - 1;
		}
	}

	public InsertPagesDlg(Juggler window) {
		builder = new Builder();
		try {
			builder.add_from_file("ui/insertPages.glade");
		} catch(Error e) {
			dialog = null;
			return; // Run() won't work if dialog == null
		}
		dialog = builder.get_object("insertPages") as Dialog;

		pageNumberEntry = builder.get_object("pageNumberEntry") as Entry;
		ofLabel = builder.get_object("ofLabel") as Label;

		locationAfterRadio = builder.get_object("locationAfterRadio") as RadioButton;
		pageFirstRadio = builder.get_object("pageFirstRadio") as RadioButton;
		pageLastRadio = builder.get_object("pageLastRadio") as RadioButton;
	}

	public bool Run() {
		if(dialog == null)
			return(false);

		ResponseType response;
		while((response = (ResponseType) dialog.run()) == ResponseType.OK) {
			selectedIndex = locationAfterRadio.active ? 1 : 0;

			if(pageFirstRadio.active)
				selectedIndex += 0;
			else if(pageLastRadio.active)
				selectedIndex += lastPageIndex;
			else
				selectedIndex += int.parse(pageNumberEntry.text) - 1;

			if(selectedIndex >= 0 && selectedIndex <= lastPageIndex + 1)
				break;
		}

		dialog.destroy();

		return(response == ResponseType.OK);				
	}
}

public class RotatePagesDlg {
	Builder builder;
	Dialog dialog;

	int selectedToIndex;
	int selectedFromIndex;
	int lastPageIndex;

	public int degrees { public get; private set; }

	public int fromIndex { 
		get {
			return(selectedFromIndex);
		}
		set {
			Entry fromEntry = builder.get_object("fromEntry") as Entry;
			fromEntry.text = (value + 1).to_string();
		}
	}
	public int toIndex { 
		get {
			return(selectedToIndex);
		}
		set {
			Entry toEntry = builder.get_object("toEntry") as Entry;
			toEntry.text = (value + 1).to_string();
		}
	}
	public int pageCount { 
		set {
			Label ofLabel = builder.get_object("ofLabel") as Label;
			ofLabel.label = "of " + value.to_string();
			lastPageIndex = value - 1;
		}
	}

	public RotatePagesDlg(Juggler window) {
		builder = new Builder();
		try {
			builder.add_from_file("ui/rotatePages.glade");
		} catch(Error e) {
			dialog = null;
			return; // Run() won't work if dialog == null
		}
		dialog = builder.get_object("rotatePages") as Dialog;
	}

	public bool Run() {
		if(dialog == null)
			return(false);

		ResponseType response;
		while((response = (ResponseType) dialog.run()) == ResponseType.OK) {
			RadioButton direction90Radio = builder.get_object("direction90Radio") as RadioButton;
			RadioButton direction180Radio = builder.get_object("direction180Radio") as RadioButton;
			if(direction90Radio.active)
				degrees = 90;
			else if(direction180Radio.active)
				degrees = 180;
			else
				degrees = 270;

			RadioButton rangeAllRadio = builder.get_object("rangeAllRadio") as RadioButton;
			if(rangeAllRadio.active) {
				selectedFromIndex = 0;
				selectedToIndex = lastPageIndex;
			} else {
				Entry fromEntry = builder.get_object("fromEntry") as Entry;
				selectedFromIndex = int.parse(fromEntry.text) - 1;
				Entry toEntry = builder.get_object("toEntry") as Entry;
				selectedToIndex = int.parse(toEntry.text) - 1;
			}

			if(selectedFromIndex >= 0 && selectedFromIndex <= lastPageIndex &&
			    selectedToIndex >= 0 && selectedToIndex <= lastPageIndex &&
				selectedFromIndex <= selectedToIndex)
			{
				break;
			}
		}

		dialog.destroy();

		return(response == ResponseType.OK);				
	}
}


public class StockDialogs {
	public static bool YesNo(Window parent, string question) {
		MessageDialog msg = new Gtk.MessageDialog(parent, Gtk.DialogFlags.MODAL, 
		    Gtk.MessageType.QUESTION, Gtk.ButtonsType.YES_NO, question);
		int response = msg.run();
		msg.destroy();

		return(response == ResponseType.YES);
	}

	public static void Error(Window parent, string text) {
		MessageDialog msg = new Gtk.MessageDialog(parent, Gtk.DialogFlags.MODAL, 
		    Gtk.MessageType.ERROR, Gtk.ButtonsType.OK, text);
		msg.run();
		msg.destroy();
	}

	public static string Open(Window parent) {
		var fileChooser = new FileChooserDialog("Open File", parent,
		    FileChooserAction.OPEN,	Stock.CANCEL, ResponseType.CANCEL,
			Stock.OPEN, ResponseType.ACCEPT);

		string filename = null;
        if(fileChooser.run() == ResponseType.ACCEPT)
			filename = fileChooser.get_filename();

		fileChooser.destroy();
		return(filename);
	}

	public static string SelectFolder(Window parent) {
		var fileChooser = new FileChooserDialog("Select folder", parent,
		    FileChooserAction.SELECT_FOLDER, Stock.CANCEL, ResponseType.CANCEL,
			Stock.SAVE, ResponseType.ACCEPT);

		string filename = null;
        if(fileChooser.run() == ResponseType.ACCEPT)
			filename = fileChooser.get_filename();

		fileChooser.destroy();
		return(filename);
	}

	public static string Save(Window parent) {
		var fileChooser = new FileChooserDialog("Save File", parent,
		    FileChooserAction.SAVE, Stock.CANCEL, ResponseType.CANCEL,
			Stock.SAVE, ResponseType.ACCEPT);

		string filename = null;
        if(fileChooser.run() == ResponseType.ACCEPT)
			filename = fileChooser.get_filename();

		fileChooser.destroy();
		return(filename);
	}		
}