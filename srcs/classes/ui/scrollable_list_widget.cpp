#include "classes/ui/scrollable_list_widget.hpp"

#include <algorithm>
#include <utility>

namespace ui {

void ScrollableListWidget::setBounds(Rectangle newBounds) {
	bounds = newBounds;
	scroll = std::clamp(scroll, 0.0f, maxScroll());
}

void ScrollableListWidget::setItemCount(std::size_t count) {
	itemCount = count;
	scroll = std::clamp(scroll, 0.0f, maxScroll());
}

void ScrollableListWidget::setRowHeight(float height) {
	rowHeight = std::max(1.0f, height);
	scroll = std::clamp(scroll, 0.0f, maxScroll());
}

void ScrollableListWidget::setRowUpdate(RowUpdate callback) {
	updateRow = std::move(callback);
}

void ScrollableListWidget::setRowDraw(RowDraw callback) {
	drawRow = std::move(callback);
}

void ScrollableListWidget::resetScroll() {
	scroll = 0.0f;
}

float ScrollableListWidget::maxScroll() const {
	const float contentHeight = static_cast<float>(itemCount) * rowHeight;
	return std::max(0.0f, contentHeight - bounds.height);
}

Rectangle ScrollableListWidget::rowBounds(std::size_t index) const {
	const bool hasScrollbar = maxScroll() > 0.0f;
	const float contentWidth = bounds.width
		- (hasScrollbar ? scrollbarWidth : 0.0f);
	return Rectangle{
		bounds.x + padding,
		bounds.y + static_cast<float>(index) * rowHeight - scroll + padding,
		std::max(1.0f, contentWidth - padding * 2.0f),
		std::max(1.0f, rowHeight - padding * 2.0f)
	};
}

bool ScrollableListWidget::rowVisible(Rectangle row) const {
	return row.y + row.height >= bounds.y
		&& row.y <= bounds.y + bounds.height;
}

bool ScrollableListWidget::update() {
	if (bounds.width <= 0.0f || bounds.height <= 0.0f)
		return false;
	if (CheckCollisionPointRec(GetMousePosition(), bounds))
		scroll = std::clamp(
			scroll - GetMouseWheelMove() * rowHeight,
			0.0f,
			maxScroll()
		);
	if (!updateRow)
		return false;

	bool changed = false;
	for (std::size_t index = 0; index < itemCount; ++index) {
		const Rectangle row = rowBounds(index);
		if (!rowVisible(row))
			continue;
		changed = updateRow(index, row) || changed;
	}
	return changed;
}

void ScrollableListWidget::draw() {
	if (bounds.width <= 0.0f || bounds.height <= 0.0f)
		return;

	DrawRectangleRec(bounds, ColorAlpha(BLUE, 0.18f));
	DrawRectangleLinesEx(bounds, 2.0f, ColorAlpha(SKYBLUE, 0.55f));
	if (drawRow) {
		BeginScissorMode(
			static_cast<int>(bounds.x),
			static_cast<int>(bounds.y),
			static_cast<int>(bounds.width),
			static_cast<int>(bounds.height)
		);
		for (std::size_t index = 0; index < itemCount; ++index) {
			const Rectangle row = rowBounds(index);
			if (rowVisible(row))
				drawRow(index, row);
		}
		EndScissorMode();
	}

	const float maximum = maxScroll();
	if (maximum <= 0.0f)
		return;
	const float trackHeight = std::max(1.0f, bounds.height - padding * 2.0f);
	const float thumbHeight = std::max(
		20.0f,
		trackHeight * bounds.height
			/ (static_cast<float>(itemCount) * rowHeight)
	);
	const float thumbTravel = std::max(0.0f, trackHeight - thumbHeight);
	const float thumbY = bounds.y + padding
		+ thumbTravel * (scroll / maximum);
	DrawRectangleRec(
		Rectangle{
			bounds.x + bounds.width - scrollbarWidth + 2.0f,
			bounds.y + padding,
			scrollbarWidth - 4.0f,
			trackHeight
		},
		ColorAlpha(BLUE, 0.6f)
	);
	DrawRectangleRec(
		Rectangle{
			bounds.x + bounds.width - scrollbarWidth + 2.0f,
			thumbY,
			scrollbarWidth - 4.0f,
			thumbHeight
		},
		ColorAlpha(SKYBLUE, 0.8f)
	);
}

} // namespace ui
