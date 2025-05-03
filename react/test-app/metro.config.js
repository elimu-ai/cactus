const { getDefaultConfig } = require('expo/metro-config');
const path = require('path');

// Find the project and workspace directories
const projectRoot = __dirname;
// This can be replaced with `find-yarn-workspace-root`
const workspaceRoot = path.resolve(projectRoot, '..');

const config = getDefaultConfig(projectRoot);

// Watch all files within the monorepo
config.watchFolders = [workspaceRoot];

// Let Metro know where to resolve packages and in what order
config.resolver.nodeModulesPaths = [
  path.resolve(projectRoot, 'node_modules'),
  path.resolve(workspaceRoot, 'node_modules'), // Allow finding the parent package's JS code
];

// Force Metro to resolve certain dependencies from the test-app's node_modules only
config.resolver.blockList = [
  // Escape forward slashes for RegExp constructor
  new RegExp(
    `${path.resolve(workspaceRoot, 'node_modules/react').replace(/\//g, '\\/')}\/.*`,
  ),
  new RegExp(
    `${path.resolve(workspaceRoot, 'node_modules/react-native').replace(/\//g, '\\/')}\/.*`,
  ),
];

config.resolver.extraNodeModules = {
  // Point cactus-react-native JS code to the parent directory's src
  // This allows Metro to find the JS source when using the "file:.." link
  'cactus-react-native': path.resolve(workspaceRoot, 'src'),
};


module.exports = config; 