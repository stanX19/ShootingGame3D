#ifndef UI_SCROLLABLE_LIST_WIDGET_HPP
#define UI_SCROLLABLE_LIST_WIDGET_HPP

#include "classes/ui/i_widget.hpp"
#include "includes.hpp"

#include <cstddef>
#include <functional>

namespace ui {

class ScrollableListWidget : public IWidget {
public:
	using RowUpdate = std::function<bool(std::size_t, Rectangle)>;
	using RowDraw = std::function<void(std::size_t, Rectangle)>;

	void setBounds(Rectangle newBounds);
	void setItemCount(std::size_t count);
	void setRowHeight(float height);
	void setRowUpdate(RowUpdate callback);
	void setRowDraw(RowDraw callback);
	void resetScroll();

	bool update() override;
	void draw() override;

private:
	static constexpr float padding = 5.0f;
	static constexpr float scrollbarWidth = 12.0f;

	float maxScroll() const;
	Rectangle rowBounds(std::size_t index) const;
	bool rowVisible(Rectangle row) const;

	Rectangle bounds = {0.0f, 0.0f, 0.0f, 0.0f};
	std::size_t itemCount = 0;
	float rowHeight = 60.0f;
	float scroll = 0.0f;
	RowUpdate updateRow;
	RowDraw drawRow;
};

} // namespace ui

#endif // UI_SCROLLABLE_LIST_WIDGET_HPP
