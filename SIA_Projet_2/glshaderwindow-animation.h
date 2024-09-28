
// à insérer dans les membres protected de la classe glShaderWindow

void keyPressEvent(QKeyEvent* e)
{
	int key = e->key();
	switch (key)
	{
	case Qt::Key_Space:
		toggleAnimating();
		break;
	default:
		break;
	}
