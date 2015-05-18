///////////////////////////////////////////////////////
// Beehive: A complete SEGA Mega Drive content tool
//
// (c) 2015 Matt Phillips, Big Evil Corporation
///////////////////////////////////////////////////////

#include "PalettesPanel.h"
#include "MainWindow.h"
#include <wx/App.h>
#include <maths/Vector.h>

PalettesPanel::PalettesPanel(	MainWindow* mainWindow,
								wxWindow *parent,
								wxWindowID id,
								const wxPoint& pos,
								const wxSize& size,
								long style,
								const wxString& name)
	: wxPanel(parent, id, pos, size, style, name)
{
	m_project = NULL;
	m_mainWindow = mainWindow;
	m_orientation = eVertical;

	Bind(wxEVT_LEFT_DOWN,		&PalettesPanel::OnMouse, this, GetId());
	Bind(wxEVT_LEFT_DCLICK,		&PalettesPanel::OnMouse, this, GetId());
	Bind(wxEVT_PAINT,			&PalettesPanel::OnPaint, this, GetId());
	Bind(wxEVT_ERASE_BACKGROUND,&PalettesPanel::OnErase, this, GetId());
	Bind(wxEVT_SIZE,			&PalettesPanel::OnResize, this, GetId());

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void PalettesPanel::OnMouse(wxMouseEvent& event)
{
	if(m_project)
	{
		//Get mouse position in map space
		wxClientDC clientDc(this);
		wxPoint mouseCanvasPosWx = event.GetLogicalPosition(clientDc);
		ion::Vector2 mousePosMapSpace(mouseCanvasPosWx.x, mouseCanvasPosWx.y);

		//Get panel size
		wxSize panelSize = GetClientSize();

		float x = (m_orientation == eVertical) ? mousePosMapSpace.y : mousePosMapSpace.x;
		float y = (m_orientation == eVertical) ? mousePosMapSpace.x : mousePosMapSpace.y;
		float colourRectSize = (m_orientation == eVertical) ? (panelSize.y / Palette::coloursPerPalette) : (panelSize.x / Palette::coloursPerPalette);

		//Get current selection
		unsigned int colourId = (unsigned int)floor(x / colourRectSize);
		unsigned int paletteId = (unsigned int)floor(y / colourRectSize);

		if(paletteId < 4 && paletteId < m_project->GetNumPalettes() && colourId < Palette::coloursPerPalette)
		{
			if(Palette* palette = m_project->GetPalette((PaletteId)paletteId))
			{
				if(event.LeftDClick())
				{
					wxColourDialog dialogue(this);
					if(dialogue.ShowModal() == wxID_OK)
					{
						wxColour wxcolour = dialogue.GetColourData().GetColour();
						Colour colour(wxcolour.Red(), wxcolour.Green(), wxcolour.Blue());
						palette->SetColour(colourId, colour);

						//Refresh tiles, stamps and map panels
						m_mainWindow->RefreshAll();
					}
				}

				if(palette->IsColourUsed(colourId))
				{
					if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
					{
						//Set current paint colour
						m_project->SetPaintColour(colourId);
					}
				}
			}
		}
	}

	event.Skip();
}

void PalettesPanel::OnPaint(wxPaintEvent& event)
{
	//Double buffered dest dc
	wxAutoBufferedPaintDC destDC(this);

	//Get renderable client rect
	wxSize clientSize = GetClientSize();
	wxRect clientRect(0, 0, clientSize.x, clientSize.y);

	//Clear dest rect
	destDC.SetBrush(*wxBLACK_BRUSH);
	destDC.DrawRectangle(clientRect);

	//No outline
	destDC.SetPen(wxNullPen);

	float colourRectSize = (m_orientation == eVertical) ? (clientSize.y / Palette::coloursPerPalette) : (clientSize.x / Palette::coloursPerPalette);

	for(int i = 0; i < m_project->GetNumPalettes(); i++)
	{
		const Palette* palette = m_project->GetPalette(i);

		for(int j = 0; j < Palette::coloursPerPalette; j++)
		{
			int x = (m_orientation == eVertical) ? i : j;
			int y = (m_orientation == eVertical) ? j : i;

			wxBrush brush;

			if(palette->IsColourUsed(j))
			{
				const Colour& colour = palette->GetColour(j);
				brush.SetColour(wxColour(colour.r, colour.g, colour.b));
			}
			else
			{
				brush.SetStyle(wxBRUSHSTYLE_CROSSDIAG_HATCH);
				brush.SetColour(wxColour(255, 0, 0, 50));
			}

			destDC.SetBrush(brush);
			destDC.DrawRectangle(x * colourRectSize, y * colourRectSize, colourRectSize, colourRectSize);
		}
	}
}

void PalettesPanel::OnErase(wxEraseEvent& event)
{
	//Ignore event
}

void PalettesPanel::OnResize(wxSizeEvent& event)
{
	if(m_project)
	{
		wxSize newSize = event.GetSize();

		if(newSize.x > newSize.y)
		{
			//Set new orientation
			m_orientation = eHorizontal;

			//Limit height
			int colourRectSize = (newSize.x / Palette::coloursPerPalette);
			SetMinSize(wxSize(1, colourRectSize * m_project->GetNumPalettes()));
		}
		else
		{
			//Set new orientation
			m_orientation = eVertical;

			//Limit width
			int colourRectSize = (newSize.y / Palette::coloursPerPalette);
			SetMinSize(wxSize(colourRectSize * m_project->GetNumPalettes(), 1));
		}
	}

	Refresh();
}