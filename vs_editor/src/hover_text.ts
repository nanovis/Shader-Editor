import * as monaco from 'monaco-editor/esm/vs/editor/editor.api';
import { createDependencyProposals } from './autocomplete/validator';

export function addHoverText() {

  monaco.languages.registerHoverProvider('mySpecialLanguage', {
    provideHover: async function (model, position) {

      const textUntilPosition = model.getValueInRange({
				startLineNumber: 1,
				startColumn: 1,
				endLineNumber: position.lineNumber,
				endColumn: position.column
			});
			const word = model.getWordUntilPosition(position);
			const range = {
				startLineNumber: position.lineNumber,
				endLineNumber: position.lineNumber,
				startColumn: word.startColumn,
				endColumn: word.endColumn
			};

      const s = await createDependencyProposals(range, textUntilPosition);
      console.log(s);
      return {
        range: new monaco.Range(
          1,
          1,
          model.getLineCount(),
          model.getLineMaxColumn(model.getLineCount())
        ),
        contents: [
          { value: '```\n' + model.getValue() + '\n```' },
          //{ value: `${s.description}` }
        ]
      };
    }
  });
}