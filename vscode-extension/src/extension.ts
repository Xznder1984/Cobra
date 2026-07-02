import * as vscode from 'vscode';

export function activate(context: vscode.ExtensionContext) {
    console.log('Cobra language extension activated');

    const statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    statusBarItem.text = "$(snake) Cobra";
    statusBarItem.tooltip = "Cobra Language";
    statusBarItem.command = "cobra.showVersion";
    statusBarItem.show();
    context.subscriptions.push(statusBarItem);

    const disposable = vscode.commands.registerCommand('cobra.showVersion', () => {
        vscode.window.showInformationMessage('Cobra Language v0.1.0');
    });
    context.subscriptions.push(disposable);

    vscode.languages.registerHoverProvider('cobra', {
        provideHover(document, position, token) {
            const range = document.getWordRangeAtPosition(position);
            if (!range) return null;
            const word = document.getText(range);
            const keywords = ['fn', 'let', 'const', 'struct', 'enum', 'trait', 'impl', 'match'];
            if (keywords.includes(word)) {
                return new vscode.Hover(`**${word}** - Cobra keyword`);
            }
            return null;
        }
    });
}

export function deactivate() {}
