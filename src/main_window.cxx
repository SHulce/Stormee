#include <iostream>
#include <gtkmm/linkbutton.h>
#include <gtkmm/widget.h>
#include <gtkmm/textview.h>
#include "main_window.hxx"
#include "util.hxx"
#include <champlain-gtk/champlain-gtk.h>

CAPViewer::Window::Window(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder) : Gtk::Window(cobject)
{

  // Setup about dialog
  refBuilder->get_widget("aboutDialog", m_aboutDialog);
  refBuilder->get_widget("imagemenuitem15", m_aboutMenuItem);
  m_aboutMenuItem->signal_activate().connect(sigc::mem_fun(*this, &CAPViewer::Window::on_aboutDialog));

  // Setup the Tree View
  refBuilder->get_widget("keyValueTreeView", keyValueTreeView);
  m_refKeyValueModel = Gtk::TreeStore::create(m_TreeViewModelColumns);
  keyValueTreeView->set_model(m_refKeyValueModel);
  keyColumn.pack_start(m_TreeViewModelColumns.m_col_key);
  valueColumn.pack_start(m_TreeViewModelColumns.m_col_value);
  keyColumn.set_expand(false);
  keyColumn.set_alignment(1.0);
  valueColumn.set_expand(true);
  
  keyValueTreeView->append_column(keyColumn);
  keyValueTreeView->append_column(valueColumn);
  keyValueTreeView->set_has_tooltip(true);
  keyValueTreeView->signal_query_tooltip().connect( sigc::mem_fun(this, &CAPViewer::Window::m_query_tooltip ));

  auto iter = m_refKeyValueModel->append();
  (*iter)[m_TreeViewModelColumns.m_col_value] = "No CAP File Loaded";

  // setup the combo box
  refBuilder->get_widget("comboBox2", m_comboBox);
  m_refComboBoxModel = Gtk::TreeStore::create(m_ComboBoxModelColumns);
  m_comboBox->set_model(m_refComboBoxModel);
  m_comboBox->pack_start(m_ComboBoxModelColumns.m_col_title);
  m_comboBox->pack_start(m_ComboBoxModelColumns.m_col_status);
  m_comboBox->pack_start(m_ComboBoxModelColumns.m_col_msgType);
  m_comboBox->pack_start(m_ComboBoxModelColumns.m_col_urgency);
  m_comboBox->pack_start(m_ComboBoxModelColumns.m_col_severity);
  m_comboBox->signal_changed()
    .connect(sigc::mem_fun(*this, &CAPViewer::Window::on_combo_changed));


  // Setup the notebook :)
  refBuilder->get_widget("notebook", m_notebook);
  tagtable = Gtk::TextTagTable::create();
  centerMonoTag = Gtk::TextTag::create("Center Mono");
  leftTag = Gtk::TextTag::create("Left Justified");
  leftMonoTag = Gtk::TextTag::create("Left Mono");
  rightTag = Gtk::TextTag::create("Right Justified");
  rightMonoTag = Gtk::TextTag::create("Right Mono");
  centerMonoTag->property_justification() = Gtk::Justification::JUSTIFY_CENTER;
  leftTag->property_justification() = Gtk::Justification::JUSTIFY_LEFT;
  leftMonoTag->property_justification() = Gtk::Justification::JUSTIFY_LEFT;
  rightTag->property_justification() = Gtk::Justification::JUSTIFY_RIGHT;
  rightMonoTag->property_justification() = Gtk::Justification::JUSTIFY_RIGHT;
  centerMonoTag->property_font() = "Monospace";
  leftMonoTag->property_font() = "Monospace";
  rightMonoTag->property_font() = "Monospace";
  leftTag->property_size_points() = 14;
  leftTag->property_weight() = Pango::Weight::WEIGHT_BOLD;
  tagtable->add(centerMonoTag);
  tagtable->add(leftTag);
  tagtable->add(leftMonoTag);
  tagtable->add(rightTag);
  tagtable->add(rightMonoTag);
  show_all_children();
}

void CAPViewer::Window::on_aboutDialog() {
  m_aboutDialog->run();
  m_aboutDialog->hide();
}

bool CAPViewer::Window::m_query_tooltip(int x, int y, bool, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
  int nx, ny, cellx, celly;
  Gtk::TreeModel::Path path;
  Gtk::TreeView::Column* column;

  keyValueTreeView->convert_widget_to_bin_window_coords(x, y, nx, ny);
  bool isRow = keyValueTreeView->get_path_at_pos(nx, ny, path, column, cellx, celly);
  if (!isRow) // Cursor is not hovering over a row
    return false;

  keyValueTreeView->set_tooltip_cell(tooltip, &path, column, 0);
  auto iter = m_refKeyValueModel->get_iter(path);
  if ( column == &keyColumn ) {
    Glib::ustring text = (*iter)[m_TreeViewModelColumns.m_col_keytip];
    if ( text.length() > 0 ) {
      tooltip->set_text(text);
      return true;
    } else {
      return false;
    }
  } 
  
  if ( column == &valueColumn ) {
    Glib::ustring text = (*iter)[m_TreeViewModelColumns.m_col_valuetip];
    if ( text.length() > 0 ) {
      tooltip->set_text(text);
      return true;
    } else {
      return false; 
    }
  }
  
  return false;
}

// Read a CAP from the tree store and populate the screen
void CAPViewer::Window::on_combo_changed() {
  auto iter = m_comboBox->get_active();
  if (iter) {
    Gtk::TreeModel::Row row = *iter;
    if (row) {
      CAPViewer::CAP cap = row[m_ComboBoxModelColumns.m_col_cap];
      // Clear out the old treeview and notebook
      m_refKeyValueModel->clear();
      for ( int i = m_notebook->get_n_pages(); i >= 0; i-- ) {
	m_notebook->remove_page(i);
      }

      
      // Populate the treeview
      addKeyValue("Note", cap.getNote(), CAPViewer::CAP::aboutNote());
      addKeyValue("Message Type", CAPViewer::msgTypeStringMap.find(cap.getMsgType())->second, CAPViewer::aboutMsgType(), CAPViewer::aboutMsgType(cap.getMsgType()));
      addKeyValue("Incidents", cap.getIncidents(), CAPViewer::CAP::aboutIncidents());
      addKeyValue("Status", CAPViewer::statusStringMap.find(cap.getStatus())->second, CAPViewer::aboutStatus(), CAPViewer::aboutStatus(cap.getStatus()));
      addKeyValue("Scope", CAPViewer::scopeStringMap.find(cap.getScope())->second, CAPViewer::aboutScope(), CAPViewer::aboutScope(cap.getScope()));
      addKeyValue("Addresses", cap.getAddresses(), CAPViewer::CAP::aboutAddresses());
      addKeyValue("Restriction", cap.getRestriction(), CAPViewer::CAP::aboutRestriction());
      addKeyValue("Source", cap.getSource(), CAPViewer::CAP::aboutSource());
      addKeyValue("Identifier", cap.getId(), CAPViewer::CAP::aboutId());
      addKeyValue("Sender", cap.getSender(), CAPViewer::CAP::aboutSender());
      addKeyValue("Sent", cap.getSent().toString(), CAPViewer::CAP::aboutSent());
      for ( auto code : cap.getCodes() ) {
	addKeyValue("Code", code, CAPViewer::CAP::aboutCodes());
      }
      for ( auto reference : cap.getReferences() ) {
	addKeyValue("Reference", reference, CAPViewer::CAP::aboutReferences());
      }
      for ( auto info : cap.getInfos() ) {
	auto infoIter = m_refKeyValueModel->append();
	(*infoIter)[m_TreeViewModelColumns.m_col_key] = "Info";
	Gtk::TreeNodeChildren children = infoIter->children();
	addKeyValueChild(children, "Sender Name", info.getSenderName(), CAPViewer::Info::aboutSenderName());
	for (auto category : info.getCategories()) {
	  addKeyValueChild(children, "Category", CAPViewer::categoryStringMap.find(category)->second, CAPViewer::aboutCategory(), CAPViewer::aboutCategory(category));
	}
	addKeyValueChild(children, "Event", info.getEvent(), CAPViewer::Info::aboutEvent());
	addKeyValueChild(children, "Urgency", CAPViewer::urgencyStringMap.find(info.getUrgency())->second, CAPViewer::aboutUrgency(), CAPViewer::aboutUrgency(info.getUrgency()));
	addKeyValueChild(children, "Severity", CAPViewer::severityStringMap.find(info.getSeverity())->second, CAPViewer::aboutSeverity(), CAPViewer::aboutSeverity(info.getSeverity()));
	addKeyValueChild(children, "Certainty", CAPViewer::certaintyStringMap.find(info.getCertainty())->second, CAPViewer::aboutCertainty(), CAPViewer::aboutCertainty(info.getCertainty()));
	for (auto response : info.getResponses()) {
	  addKeyValueChild(children, "Response", CAPViewer::responseStringMap.find(response)->second, CAPViewer::aboutResponse(), CAPViewer::aboutResponse(response));
	}
	addKeyValueChild(children, "Audience", info.getAudience(), info.aboutAudience());
	if ( !info.getEventCodes().empty() ) {
	  auto codesIter = m_refKeyValueModel->append(children);
	  (*codesIter)[m_TreeViewModelColumns.m_col_key] = "Event Codes";
	  (*codesIter)[m_TreeViewModelColumns.m_col_keytip] = CAPViewer::Info::aboutEventCodes();
	  Gtk::TreeNodeChildren codeChildren = codesIter->children();
	  
	  for (auto codepair : info.getEventCodes()) {
	    addKeyValueChild(codeChildren, codepair.first, codepair.second);
	  }
	}
	addKeyValueChild(children, "Effective", info.getEffective().toString(), info.aboutEffective());
	addKeyValueChild(children, "Onset", info.getOnset().toString(), info.aboutOnset());
	addKeyValueChild(children, "Expires", info.getExpires().toString(), info.aboutExpires());
	addKeyValueChild(children, "Web", info.getWeb(), info.aboutWeb());
	addKeyValueChild(children, "Contact", info.getContact(), info.aboutContact());

	if ( !info.getParameters().empty() ) {
	  auto parameterIter = m_refKeyValueModel->append(children);
	  (*parameterIter)[m_TreeViewModelColumns.m_col_key] = "Parameters";
	  (*parameterIter)[m_TreeViewModelColumns.m_col_keytip] = CAPViewer::Info::aboutParameters();
	  Gtk::TreeNodeChildren parameterChildren = parameterIter->children();
	  for ( auto parameterpair : info.getParameters()) {
	    addKeyValueChild(parameterChildren, parameterpair.first, parameterpair.second);
	  }
	}
	// Populate the Notebook

	// First, the Description/Instruction
	Glib::RefPtr<Gtk::TextBuffer> tb = Gtk::TextBuffer::create(tagtable);
	auto iter = tb->begin();
	iter = tb->insert_with_tag(iter, info.getHeadline(), centerMonoTag);
	iter = tb->insert_with_tag(iter, "\n\nDescription:\n", leftTag);
	iter = tb->insert_with_tag(iter, info.getDescription(), leftMonoTag);
	iter = tb->insert_with_tag(iter, "\n\nInstruction:\n", leftTag);
	iter = tb->insert_with_tag(iter, info.getInstruction(), leftMonoTag);
	Gtk::TextView* tv = Gtk::manage(new Gtk::TextView(tb));
	tv->set_editable(false);
	tv->set_wrap_mode(Gtk::WrapMode::WRAP_WORD_CHAR);
	auto tv_sw = Gtk::manage(new Gtk::ScrolledWindow());
	tv_sw->set_policy(Gtk::PolicyType::POLICY_NEVER, Gtk::PolicyType::POLICY_AUTOMATIC);
	tv_sw->set_min_content_width(0);
	tv_sw->add(*tv);
	m_notebook->append_page(*tv_sw, "Info");
	
	// Second, the the maps
	for ( auto area : info.getAreas() ) {
	  auto polys = area.getPolygons();
	  auto circles = area.getCircles();
	  if ( polys.size() > 0 || circles.size() > 0 ) {
	    GtkWidget* cham_raw = gtk_champlain_embed_new();
	    ChamplainView* cham_view = gtk_champlain_embed_get_view( GTK_CHAMPLAIN_EMBED( cham_raw ) );

	    for ( uint i = 0; i < polys.size(); i++ ) {
	      ChamplainPathLayer* path = champlain_path_layer_new();
	      guint counter = 0;
	      for ( auto coord : polys[i].getPoints() ) {
		ChamplainCoordinate* location = champlain_coordinate_new_full(coord.getLatitude(), coord.getLongitude());
		champlain_path_layer_insert_node(path, CHAMPLAIN_LOCATION( location ), counter++);
	      }
	      champlain_path_layer_set_closed(path, true);
	      champlain_path_layer_set_fill(path, true);
	      champlain_view_add_layer( cham_view, CHAMPLAIN_LAYER(path) );
	    }
	    
	    champlain_view_ensure_layers_visible(cham_view, false);
	    champlain_view_set_zoom_level(cham_view, champlain_view_get_zoom_level(cham_view) + 3);

	    for ( uint i = 0; i < circles.size(); i++ ) {
	      // TODO: Display circles
	    }

	    Gtk::Widget* cham = Gtk::manage(Glib::wrap(cham_raw));
	    m_notebook->append_page(*cham, "Map");
	  }
	} // for Areas()
	
	// Now the resources
	for (auto resource : info.getResources()) {
	  auto grid = Gtk::manage(new Gtk::Grid());
	  grid->attach(*Gtk::manage(new Gtk::Label("Description: ", 1.0F, .5F)), 0, 0, 1, 1);
	  grid->attach(*Gtk::manage(new Gtk::Label(resource.getResourceDesc(), 0.0F, .5F)), 1, 0, 1, 1);
	  grid->attach(*Gtk::manage(new Gtk::Label("MIME Type: ", 1.0F, .5F)), 0, 1, 1, 1);
	  grid->attach(*Gtk::manage(new Gtk::Label(resource.getMimeType(), 0.0F, .5F)), 1, 1, 1, 1);
	  grid->attach(*Gtk::manage(new Gtk::Label("Size: ", 1.0F, .5F)), 0, 2, 1, 1);
	  grid->attach(*Gtk::manage(new Gtk::Label(Glib::ustring::compose("%1 bytes", resource.getSize()), 0.0F, .5F)), 1, 2, 1, 1);
	  if (resource.getDerefUri().size() == 0) {
	    grid->attach(*Gtk::manage(new Gtk::Label("URL: ", 1.0F, .5F)), 0, 3, 1, 1);
	    // There's some bug in Fedora 16's gtkmm where using the 1 var constructor won't link
	    auto mmlb = Gtk::manage(new Gtk::LinkButton(resource.getUri(), resource.getUri()));
	    mmlb->set_relief(Gtk::ReliefStyle::RELIEF_NONE);
	    mmlb->set_alignment(0.0F, .5F);
	    grid->attach(*mmlb, 1, 3, 1, 1);
	  } else {
	    // TODO: Implement
	    grid->attach(*Gtk::manage(new Gtk::Label("Dereferenced URIs are not supported")), 0, 3, 2, 1);
	  }
	  // TODO: Check the digest
	  m_notebook->append_page(*grid, "Resource");
	
	} // for Resources()
  
    } // for Infos()

      keyValueTreeView->expand_all();
      m_notebook->show_all_children();
    }
  }
}

void CAPViewer::Window::addKeyValue(const Glib::ustring& key, const Glib::ustring& value, const Glib::ustring& keyTooltip, const Glib::ustring& valueTooltip) {
  if (value.length() > 0) {
    auto iter = m_refKeyValueModel->append();
    (*iter)[m_TreeViewModelColumns.m_col_key] = key;
    (*iter)[m_TreeViewModelColumns.m_col_value] = value;
    (*iter)[m_TreeViewModelColumns.m_col_keytip] = keyTooltip;
    (*iter)[m_TreeViewModelColumns.m_col_valuetip] = valueTooltip;
  }
}

void CAPViewer::Window::addKeyValueChild(const Gtk::TreeNodeChildren& parent, const Glib::ustring& key, const Glib::ustring& value, const Glib::ustring& keyTooltip, const Glib::ustring& valueTooltip) {
  if (value.length() > 0) {
    auto iter = m_refKeyValueModel->append(parent);
    (*iter)[m_TreeViewModelColumns.m_col_key] = key;
    (*iter)[m_TreeViewModelColumns.m_col_value] = value;
    (*iter)[m_TreeViewModelColumns.m_col_keytip] = keyTooltip;
    (*iter)[m_TreeViewModelColumns.m_col_valuetip] = valueTooltip;
  }
}

void CAPViewer::Window::on_button_quit() {
  hide();
}

// Called from CAPViewer::Application when it gets a CAP from XMPP
// Also should be called from OpenFromFile
void CAPViewer::Window::display_cap(const CAPViewer::CAP& cap) {
  
  Gtk::TreeModel::Row row = *(m_refComboBoxModel->append());
  row[m_ComboBoxModelColumns.m_col_title] = cap.getTitle();
  row[m_ComboBoxModelColumns.m_col_status] = CAPViewer::statusStringMap.find(cap.getStatus())->second;
  row[m_ComboBoxModelColumns.m_col_msgType] = CAPViewer::msgTypeStringMap.find(cap.getMsgType())->second;
  if (cap.getInfos().size() > 0) {
    row[m_ComboBoxModelColumns.m_col_urgency] = CAPViewer::urgencyStringMap.find(cap.getInfos().front().getUrgency())->second;
    row[m_ComboBoxModelColumns.m_col_severity] = CAPViewer::severityStringMap.find(cap.getInfos().front().getSeverity())->second;
  } else {
    row[m_ComboBoxModelColumns.m_col_urgency] = CAPViewer::urgencyStringMap.find(CAPViewer::eUrgency::uUnknown)->second;
    row[m_ComboBoxModelColumns.m_col_severity] = CAPViewer::severityStringMap.find(CAPViewer::eSeverity::sUnknown)->second;
  }
  row[m_ComboBoxModelColumns.m_col_cap] = cap;
  
  if (m_comboBox->get_active_row_number() == -1)
    m_comboBox->set_active(0);
}

