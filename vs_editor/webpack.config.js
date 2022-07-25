const path = require('path');
const MonacoWebpackPlugin = require('monaco-editor-webpack-plugin');

module.exports = {
	mode: 'development',
	entry: {
		app: './src/index.js',
		'editor.worker': 'monaco-editor/esm/vs/editor/editor.worker.js',
	},
	output: {
		globalObject: 'self',
		filename: '[name].bundle.js',
		path: path.resolve(__dirname, '../out/assets/js/vs_editor')
	},
	resolve: {
		fallback: { "path": require.resolve("path-browserify") }
	},
	devtool: 'eval-source-map',
	module: {
		rules: [
			{
				test: /\.css$/,
				use: ['style-loader', 'css-loader']
			},
			{
				test: /\.ttf$/,
				use: ['file-loader']
			},
			{
				test: /\.(js)$/,
				exclude: /node_modules/,
				use: [{
					loader: 'babel-loader',
					options: {
						presets: [
							['@babel/preset-env', { targets: "defaults" }]
						]
					}
				}]
			}
		]
	},
	plugins: [
		new MonacoWebpackPlugin({ languages: [] })
	]

};