
// à insérer dans les membres public de la classe OpenGLWindow

void setAnimating(bool animating);
bool getAnimating() { return m_animating; }
void toggleAnimating() {
	m_animating = !m_animating;
	m_update_pending = false;
	renderNow();
}
