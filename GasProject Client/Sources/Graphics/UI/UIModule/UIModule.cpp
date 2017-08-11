#include "UIModule.hpp"

#include "Graphics/UI/Widget/Widget.hpp"

UIModule::UIModule(UI *ui) : ui(ui) {
	curInputWidget = nullptr;
}

void UIModule::Draw(sf::RenderWindow *renderWindow) {
	for (auto &widget : widgets)
		widget->Draw(*renderWindow);
}

void UIModule::Update(sf::Time timeElapsed) {
	for (auto &widget : widgets)
		widget->Update(timeElapsed);
}

void UIModule::HandleEvent(sf::Event event) {
	switch (event.type) {
	case sf::Event::MouseButtonPressed: {
		for (auto &widget : widgets)
			if (widget->HandleEvent(event)) {
				curInputWidget = widget.get();
				return;
			}
		break;
	}
	case sf::Event::MouseMoved: {
		for (auto &widget : widgets) {
			if (widget->HandleEvent(event))
				return;
		}
		break;
	}
	default:
		if (curInputWidget)
			curInputWidget->HandleEvent(event);
	}
}