import * as monaco from "monaco-editor/esm/vs/editor/editor.api";
import { ValidationRegexs } from "./regex_list";


export async function createDependencyProposals(range: monaco.IRange, textUntilPosition: string) {
	// returning a static list of proposals, not even looking at the prefix (filtering is done by the Monaco editor),
	// here you could do a server side lookup
	const autocomplete_json: any[] = await (await fetch(`/assets/static/configurations/WGSL.autocomplete.json`)).json();
	if (textUntilPosition.match(/@(builtin)\(\s*([A-Za-z])*?\)/)) return autocomplete_json['builtin' as any];
	if (textUntilPosition.match(/@(stage)\(\s*([A-Za-z])*?\)/)) return autocomplete_json['stage' as any];
	
	
	return [];
}

export function startValidationServer() {
	monaco.languages.registerCompletionItemProvider('wgsl', {
		provideCompletionItems: async function (model, position) {
			// find out if we are completing a property in the 'dependencies' object.
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
			return {
				suggestions: await createDependencyProposals(range, textUntilPosition)
			};
		}
	});
}