#include "classes/ui/scrollable_list_widget.hpp"

#include <algorithm>
#include <utility>

namespace ui {

void ScrollableListWidget::setBounds(Rectangle newBounds) {
	m_bounds = newBounds;
	m_scroll = std::clamp(m_scroll, 0.0f, maxScroll());
}

void ScrollableListWidget::setItemCount(std::size_t count) {
	m_itemCount = count;
	m_scroll = std::clamp(m_scroll, 0.0f, maxScroll());
}

void ScrollableListWidget::setRowHeight(float height) {
	m_rowHeight = std::max(1.0f, height);
	m_scroll = std::clamp(m_scroll, 0.0f, maxScroll());
}

void ScrollableListWidget::setRowUpdate(RowUpdate callback) {
	m_updateRow = std::move(callback);
}

void ScrollableListWidget::setRowDraw(RowDraw callback) {
	m_drawRow = std::move(callback);
}

void ScrollableListWidget::resetScroll() {
	m_scroll = 0.0f;
}

float ScrollableListWidget::maxScroll() const {
	const float contentHeight = static_cast<float>(m_itemCount) * m_rowHeight;
	return std::max(0.0f, contentHeight - m_bounds.height);
}

Rectangle ScrollableListWidget::rowBounds(std::size_t index) const {
	const bool hasScrollbar = maxScroll() > 0.0f;
	const float contentWidth = m_bounds.width
		- (hasScrollbar ? SCROLLBAR_WIDTH : 0.0f);
	return Rectangle{
		m_bounds.x + PADDING,
		m_bounds.y + static_cast<float>(index) * m_rowHeight - m_scroll + PADDING,
		std::max(1.0f, contentWidth - PADDING * 2.0f),
		std::max(1.0f, m_rowHeight - PADDING * 2.0f)
	};
}

bool ScrollableListWidget::rowVisible(Rectangle row) const {
	return row.y + row.height >= m_bounds.y
		&& row.y <= m_bounds.y + m_bounds.height;
}

bool ScrollableListWidget::update() {
	if (m_bounds.width <= 0.0f || m_bounds.height <= 0.0f)
		return false;
	if (CheckCollisionPointRec(GetMousePosition(), m_bounds))
		m_scroll = std::clamp(
			m_scroll - GetMouseWheelMove() * m_rowHeight,
			0.0f,
			maxScroll()
		);
	if (!m_updateRow)
		return false;

	bool changed = false;
	for (std::size_t index = 0; index < m_itemCount; ++index) {
		const Rectangle row = rowBounds(index);
		if (!rowVisible(row))
			continue;
		changed = m_updateRow(index, row) || changed;
	}
	return changed;
}

void ScrollableListWidget::draw() {
	if (m_bounds.width <= 0.0f || m_bounds.height <= 0.0f)
		return;

	DrawRectangleRec(m_bounds, ColorAlpha(BLUE, 0.18f));
	DrawRectangleLinesEx(m_bounds, 2.0f, ColorAlpha(SKYBLUE, 0.55f));
	if (m_drawRow) {
		BeginScissorMode(
			static_cast<int>(m_bounds.x),
			static_cast<int>(m_bounds.y),
			static_cast<int>(m_bounds.width),
			static_cast<int>(m_bounds.height)
		);
		for (std::size_t index = 0; index < m_itemCount; ++index) {
			const Rectangle row = rowBounds(index);
			if (rowVisible(row))
				m_drawRow(index, row);
		}
		EndScissorMode();
	}

	const float maximum = maxScroll();
	if (maximum <= 0.0f)
		return;
	const float trackHeight = std::max(1.0f, m_bounds.height - PADDING * 2.0f);
	const float thumbHeight = std::max(
		20.0f,
		trackHeight * m_bounds.height
			/ (static_cast<float>(m_itemCount) * m_rowHeight)
	);
	const float thumbTravel = std::max(0.0f, trackHeight - thumbHeight);
	const float thumbY = m_bounds.y + PADDING
		+ thumbTravel * (m_scroll / maximum);
	DrawRectangleRec(
		Rectangle{
			m_bounds.x + m_bounds.width - SCROLLBAR_WIDTH + 2.0f,
			m_bounds.y + PADDING,
			SCROLLBAR_WIDTH - 4.0f,
			trackHeight
		},
		ColorAlpha(BLUE, 0.6f)
	);
	DrawRectangleRec(
		Rectangle{
			m_bounds.x + m_bounds.width - SCROLLBAR_WIDTH + 2.0f,
			thumbY,
			SCROLLBAR_WIDTH - 4.0f,
			thumbHeight
		},
		ColorAlpha(SKYBLUE, 0.8f)
	);
}

} // namespace ui
