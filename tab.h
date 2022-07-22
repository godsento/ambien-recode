#pragma once

#define ELEMENT_BORDER				35
#define ELEMENT_COLUMNS				2
#define ELEMENT_DISTANCE			10
#define ELEMENT_COMBINED_DISTANCE	7
#define ELEMENT_WIDTH				240
#define ELEMENT_COLUMN_SEPARATOR	70

class Tab {
	friend class Form;
	friend class GUI;
	friend class Config;

private:
	std::string		                   m_desc;
	std::array< int, ELEMENT_COLUMNS > m_element_offset;
	Form* m_parent;

public:
	std::vector< Element* >            m_elements;
	std::string		                   m_title;
	__forceinline Tab() : m_title{}, m_elements{}, m_element_offset{ 20, 20 }, m_parent{} {}

	// set the title of the tab.
	__forceinline void SetTitle(const std::string& title, const std::string& desc) {
		m_title = title;
		m_desc = desc;
	}

	// add a new element to the tab.
	__forceinline void RegisterElement(Element* element, size_t col = 0) {
		element->m_col = col;

		// set x.
		element->m_pos.x = ELEMENT_BORDER + (element->m_col * (ELEMENT_COLUMN_SEPARATOR + ELEMENT_WIDTH));

		// todo; dynamically or a 2nd column?
		element->m_w = ELEMENT_WIDTH;

		// set owner of element.
		element->m_parent = m_parent;

		// add element to our container.
		m_elements.push_back(element);
	}

	Element* GetElementByName(const std::string& file_id) const {
		auto it = std::find_if(m_elements.begin(), m_elements.end(),
			[&file_id](Element* elem) {
				return !elem->m_file_id.compare(file_id);
			});

		if (it != m_elements.end())
			return *it;

		return nullptr;
	}
};