#ifdef __cplusplus
extern "C" {
#endif
Widget CodaEditor(Widget toplevel/*void *topLevel*/, int withExit);
void EditorSelectConfig (char *confn);
void EditorSelectExp (Widget w, char *exp);
void setCompState(char *name,int state);

Widget CodaEditor1(Widget toplevel);

#ifdef __cplusplus
}
#endif
