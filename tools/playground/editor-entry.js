import { Compartment, EditorState } from '@codemirror/state';
import { EditorView, keymap, lineNumbers, highlightActiveLine, highlightActiveLineGutter, drawSelection } from '@codemirror/view';
import { StreamLanguage, bracketMatching, indentOnInput, foldGutter, foldKeymap } from '@codemirror/language';
import { defaultKeymap, history, historyKeymap, indentWithTab } from '@codemirror/commands';
import { oneDark } from '@codemirror/theme-one-dark';
import { swift } from '@codemirror/legacy-modes/mode/swift';

export function createSwiftEditor(parent, { doc, readOnly = false, onChange }) {
  let replacing = false;
  const access = new Compartment();
  const accessMode = value => [EditorState.readOnly.of(value), EditorView.editable.of(!value)];
  const view = new EditorView({
    parent,
    state: EditorState.create({
      doc,
      extensions: [
        lineNumbers(), highlightActiveLineGutter(), highlightActiveLine(), drawSelection(),
        history(), foldGutter(), indentOnInput(), bracketMatching(), StreamLanguage.define(swift), oneDark,
        keymap.of([indentWithTab, ...defaultKeymap, ...historyKeymap, ...foldKeymap]),
        EditorState.tabSize.of(4), access.of(accessMode(readOnly)), EditorView.lineWrapping,
        EditorView.contentAttributes.of({ 'aria-label': 'ContentView Swift source' }),
        EditorView.theme({
          '&': { height: '530px', fontSize: '13px', backgroundColor: '#0b131a' },
          '.cm-scroller': { fontFamily: 'ui-monospace, SFMono-Regular, monospace', lineHeight: '1.65' },
          '.cm-content': { padding: '14px 0' },
          '.cm-gutters': { backgroundColor: '#101b23', borderRight: '1px solid #344951' },
          '&.cm-focused': { outline: '2px solid #88e6b3', outlineOffset: '2px' },
        }),
        EditorView.updateListener.of(update => {
          if (update.docChanged && !replacing) onChange();
        }),
      ],
    }),
  });
  return {
    setReadOnly(value) { view.dispatch({ effects: access.reconfigure(accessMode(value)) }); },
    get value() { return view.state.doc.toString(); },
    set value(value) {
      replacing = true;
      try { view.dispatch({ changes: { from: 0, to: view.state.doc.length, insert: value } }); }
      finally { replacing = false; }
    },
  };
}
