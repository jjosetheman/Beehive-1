///////////////////////////////////////////////////////
// MD Studio: A complete SEGA Mega Drive content tool
//
// (c) 2015 Matt Phillips, Big Evil Corporation
///////////////////////////////////////////////////////

#include "MapPanel.h"
#include "TileRendering.h"

MapPanel::MapPanel(wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
	: wxPanel(parent, winid, pos, size, style, name)
	, m_canvas(Map::defaultWidth * 8, Map::defaultHeight * 8)
{
	m_project = NULL;
	m_cameraZoom = 1.0f;
	m_cameraPanSpeed = 1.0f;

	Bind(wxEVT_LEFT_DOWN,		&MapPanel::OnMouse, this, wxID_MAPPANEL);
	Bind(wxEVT_LEFT_UP,			&MapPanel::OnMouse, this, wxID_MAPPANEL);
	Bind(wxEVT_MIDDLE_DOWN,		&MapPanel::OnMouse, this, wxID_MAPPANEL);
	Bind(wxEVT_MIDDLE_UP,		&MapPanel::OnMouse, this, wxID_MAPPANEL);
	Bind(wxEVT_RIGHT_DOWN,		&MapPanel::OnMouse, this, wxID_MAPPANEL);
	Bind(wxEVT_RIGHT_UP,		&MapPanel::OnMouse, this, wxID_MAPPANEL);
	Bind(wxEVT_MOTION,			&MapPanel::OnMouse, this, wxID_MAPPANEL);
	Bind(wxEVT_MOUSEWHEEL,		&MapPanel::OnMouse, this, wxID_MAPPANEL);
	Bind(wxEVT_PAINT,			&MapPanel::OnPaint, this, wxID_MAPPANEL);
	Bind(wxEVT_ERASE_BACKGROUND,&MapPanel::OnErase, this, wxID_MAPPANEL);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void MapPanel::SetProject(Project* project)
{
	m_project = project;

	//Centre camera on canvas
	wxSize clientSize = GetClientSize();
	m_cameraPos.x = (clientSize.x / 2.0f) - ((m_project->GetMap().GetWidth() * 8.0f) / 2.0f);
	m_cameraPos.y = (clientSize.y / 2.0f) - ((m_project->GetMap().GetHeight() * 8.0f) / 2.0f);

	//Reset zoom
	m_cameraZoom = 1.0f;

	//Invalidate panel
	m_project->InvalidateMap(true);
	Refresh();
}

void MapPanel::OnMouse(wxMouseEvent& event)
{
	if(m_project)
	{
		//Get mouse delta
		ion::Vector2 mousePos(event.GetX(), event.GetY());
		ion::Vector2 mouseDelta = m_mousePrevPos - mousePos;
		m_mousePrevPos = mousePos;

		//Get mouse position in canvas and map space
		wxClientDC clientDc(this);
		wxPoint mouseCanvasPosWx = event.GetLogicalPosition(clientDc);

		ion::Vector2 mousePosCanvasSpace(mouseCanvasPosWx.x, mouseCanvasPosWx.y);
		ion::Vector2 mousePosMapSpace((mouseCanvasPosWx.x - m_cameraPos.x) / m_cameraZoom, (mouseCanvasPosWx.y - m_cameraPos.y) / m_cameraZoom);

		//Panting/erasing
		if(		mousePosMapSpace.x >= 0.0f
			&&	mousePosMapSpace.y >= 0.0f
			&&	mousePosMapSpace.x < (m_project->GetMap().GetWidth() * 8)
			&&	mousePosMapSpace.y < (m_project->GetMap().GetHeight() * 8))
		{
			if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
			{
				//Paint
				if(m_project->GetPaintTile())
					PaintTile(mousePosMapSpace, m_project->GetPaintTile());
			}
			else if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
			{
				//Erase
				if(m_project->GetEraseTile())
					PaintTile(mousePosMapSpace, m_project->GetEraseTile());
			}
		}

		//Camera pan/zoom
		if(event.Dragging() && event.ButtonIsDown(wxMOUSE_BTN_MIDDLE))
		{
			//Pan camera
			m_cameraPos.x += mouseDelta.x * m_cameraPanSpeed;
			m_cameraPos.y += mouseDelta.y * m_cameraPanSpeed;

			//Invalidate whole frame
			Refresh();
		}
		else if(event.GetWheelRotation() != 0)
		{
			//Zoom camera
			int wheelDelta = event.GetWheelRotation();
			const float zoomSpeed = 1.0f;

			//One notch at a time
			if(wheelDelta > 0)
				m_cameraZoom += zoomSpeed;
			else if(wheelDelta < 0)
				m_cameraZoom -= zoomSpeed;

			//Clamp
			if(m_cameraZoom < 1.0f)
				m_cameraZoom = 1.0f;
			else if(m_cameraZoom > 10.0f)
				m_cameraZoom = 10.0f;

			//Invalidate whole frame
			Refresh();
		}
	}

	event.Skip();
}

void MapPanel::OnPaint(wxPaintEvent& event)
{
	//Source context
	wxMemoryDC sourceContext(m_canvas);

	//Double buffered dest context
	wxAutoBufferedPaintDC destContext(this);

	//TODO: Transform by window scroll

	//Update all invalidated regions
	for(wxRegionIterator it = GetUpdateRegion(); it; it++)
	{
		//Get invalidated rect
		wxRect sourceRect(it.GetRect());
		wxRect destRect(sourceRect);

		//Clear rect
		destContext.SetBrush(*wxLIGHT_GREY_BRUSH);
		destContext.DrawRectangle(destRect);

		//Transform by camera pan/zoom
		sourceRect.x -= m_cameraPos.x;
		sourceRect.y -= m_cameraPos.y;
		sourceRect.x /= m_cameraZoom;
		sourceRect.y /= m_cameraZoom;
		sourceRect.width /= m_cameraZoom;
		sourceRect.height /= m_cameraZoom;
		
		//Copy rect from canvas
		destContext.StretchBlit(destRect.x, destRect.y, destRect.width, destRect.height, &sourceContext, sourceRect.x, sourceRect.y, sourceRect.width, sourceRect.height);
	}
}

void MapPanel::OnErase(wxEraseEvent& event)
{
	//Ignore event
}

void MapPanel::Refresh(bool eraseBackground, const wxRect *rect)
{
	if(m_project->MapIsInvalidated())
	{
		//Full refresh, redraw map to canvas
		wxMemoryDC dc(m_canvas);
		PaintMapToDc(dc);
		m_project->InvalidateMap(false);
	}

	wxPanel::Refresh(eraseBackground, rect);
}

void MapPanel::PaintTile(ion::Vector2 mousePos, TileId tileId)
{
	if(m_project)
	{
		int x = (int)floor(mousePos.x / 8.0f);
		int y = (int)floor(mousePos.y / 8.0f);

		//Set new tile in map
		m_project->GetMap().SetTile(x, y, tileId);

		//Paint tile to canvas dc
		if(const Tile* tile = m_project->GetMap().GetTileset().GetTile(tileId))
		{
			if(const Palette* palette = m_project->GetPalette(tile->GetPaletteId()))
			{
				wxMemoryDC dc(m_canvas);
				tilerendering::PaintTileToDc(x, y, *tile, *palette, dc);
			}
		}
		
		//Invalidate screen rect
		wxRect refreshRect(	((x * 8) * m_cameraZoom) + m_cameraPos.x,
							((y * 8) * m_cameraZoom) + m_cameraPos.y,
							8 * m_cameraZoom,
							8 * m_cameraZoom);

		RefreshRect(refreshRect);
	}
}

void MapPanel::PaintMapToDc(wxDC& dc)
{
	if(m_project)
	{
		for(int x = 0; x < m_project->GetMap().GetWidth(); x++)
		{
			for(int y = 0; y < m_project->GetMap().GetHeight(); y++)
			{
				TileId tileId = m_project->GetMap().GetTile(x, y);
				if(const Tile* tile = m_project->GetMap().GetTileset().GetTile(tileId))
				{
					if(const Palette* palette = m_project->GetPalette(tile->GetPaletteId()))
					{
						tilerendering::PaintTileToDc(x, y, *tile, *palette, dc);
					}
				}
			}
		}
	}
}