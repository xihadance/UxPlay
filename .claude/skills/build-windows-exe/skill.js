#!/usr/bin/env node

/**
 * Claude Code Skill: Build UxPlay Windows EXE
 *
 * This skill provides commands to build UxPlay for Windows from WSL
 * using cross-compilation with the MinGW-w64 toolchain.
 */

import { spawn, execSync } from 'child_process';
import { readFileSync, existsSync } from 'fs';
import { join, resolve } from 'path';
import { fileURLToPath } from 'url';
import { dirname } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Skill metadata
export const skill = {
  name: 'build-windows-exe',
  description: 'Build UxPlay Windows executable from WSL',
  version: '1.0.0',
  commands: {
    'build-windows': {
      description: 'Build UxPlay for Windows from WSL using cross-compilation',
      usage: 'build-windows [source-path] [options]',
      options: [
        {
          name: '--clean',
          description: 'Clean build directory before building',
        },
        {
          name: '--dependencies',
          description: 'Install WSL dependencies only',
        },
        {
          name: '--verbose',
          description: 'Enable verbose output',
        },
      ],
      examples: [
        {
          command: 'build-windows',
          description: 'Full build of UxPlay for Windows',
        },
        {
          command: 'build-windows /path/to/source',
          description: 'Build from specific source directory',
        },
        {
          command: 'build-windows --clean',
          description: 'Clean and rebuild',
        },
        {
          command: 'build-windows --dependencies',
          description: 'Install only build dependencies',
        },
      ],
    },
  },
  requirements: {
    os: ['linux'], // WSL (Ubuntu, etc.)
    packages: ['bash', 'cmake', 'ninja', 'git', 'mingw-w64-tools'],
  },
};

/**
 * Check if running in WSL
 */
function isWSL() {
  try {
    const version = execSync('cat /proc/version', { encoding: 'utf8' });
    return version.toLowerCase().includes('microsoft') ||
           version.toLowerCase().includes('wsl');
  } catch {
    return false;
  }
}

/**
 * Check for Bonjour SDK
 */
function checkBonjourSDK() {
  const possiblePaths = [
    '/mnt/c/Program Files/Bonjour SDK',
    '/mnt/c/Program Files (x86)/Bonjour SDK',
    process.env.BONJOUR_SDK_HOME,
  ].filter(Boolean);

  for (const path of possiblePaths) {
    try {
      execSync(`test -d "${path}"`, { shell: '/bin/bash' });
      return path;
    } catch {
      // Path doesn't exist, try next
    }
  }
  return null;
}

/**
 * Install WSL dependencies
 */
async function installDependencies(verbose = false) {
  console.log('📦 Installing WSL build dependencies...\n');

  const commands = [
    // Update package lists
    'sudo apt-get update',
    // Install cross-compilation toolchain
    'sudo apt-get install -y gcc-mingw-w64 g++-mingw-w64 mingw-w64-tools',
    // Install build tools
    'sudo apt-get install -y cmake ninja-build git wget pkg-config',
    // Install GStreamer development libraries
    'sudo apt-get install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev',
    // Install OpenSSL and libplist development files
    'sudo apt-get install -y libssl-dev libplist-dev libplist-utils',
  ];

  for (const cmd of commands) {
    console.log(`> ${cmd}`);
    try {
      execSync(cmd, {
        stdio: verbose ? 'inherit' : 'pipe',
        shell: '/bin/bash'
      });
    } catch (error) {
      console.error(`❌ Failed to execute: ${cmd}`);
      console.error(error.message);
      throw error;
    }
  }

  console.log('\n✅ Dependencies installed successfully');
}

/**
 * Clean build directory
 */
async function cleanBuild(buildDir = 'build') {
  console.log(`🧹 Cleaning build directory: ${buildDir}...\n`);

  const cmd = `rm -rf "${buildDir}"`;
  console.log(`> ${cmd}`);

  try {
    execSync(cmd, { shell: '/bin/bash' });
    console.log('✅ Build directory cleaned\n');
  } catch (error) {
    console.error('❌ Failed to clean build directory');
    throw error;
  }
}

/**
 * Configure and build UxPlay
 */
async function buildUxPlay(sourcePath = '.', verbose = false) {
  const absoluteSourcePath = resolve(process.cwd(), sourcePath);

  console.log(`🔨 Building UxPlay for Windows...`);
  console.log(`   Source: ${absoluteSourcePath}`);
  console.log(`   Working directory: ${process.cwd()}\n`);

  // Check prerequisites
  if (!isWSL()) {
    console.warn('⚠️  Warning: This script is designed for WSL. Running on native Linux may have issues.');
  }

  const bonjourPath = checkBonjourSDK();
  if (bonjourPath) {
    console.log(`✅ Bonjour SDK found: ${bonjourPath}`);
  } else {
    console.warn('⚠️  Bonjour SDK not found. DNS-SD/mDNS will not work.');
    console.warn('   Install from: https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml');
  }

  // Create build directory
  const buildDir = 'build';
  console.log(`📁 Creating build directory: ${buildDir}`);
  execSync(`mkdir -p "${buildDir}"`, { shell: '/bin/bash' });

  // Configure with CMake
  console.log('\n⚙️  Configuring with CMake...');
  const cmakeCmd = `cd "${buildDir}" && cmake "${absoluteSourcePath}" -DCMAKE_TOOLCHAIN_FILE=../windows-toolchain.cmake -G Ninja -DCMAKE_BUILD_TYPE=Release`;
  console.log(`> ${cmakeCmd}`);

  try {
    execSync(cmakeCmd, {
      stdio: verbose ? 'inherit' : 'pipe',
      shell: '/bin/bash'
    });
    console.log('✅ CMake configuration successful\n');
  } catch (error) {
    console.error('❌ CMake configuration failed');
    console.error(error.stderr?.toString() || error.message);
    throw error;
  }

  // Build
  console.log('🏗️  Building UxPlay...');
  const buildCmd = `cd "${buildDir}" && ninja`;
  console.log(`> ${buildCmd}`);

  try {
    execSync(buildCmd, {
      stdio: verbose ? 'inherit' : 'pipe',
      shell: '/bin/bash'
    });
    console.log('✅ Build successful\n');
  } catch (error) {
    console.error('❌ Build failed');
    console.error(error.stderr?.toString() || error.message);
    throw error;
  }

  // Package
  await packageBuild(buildDir);
}

/**
 * Package the build results
 */
async function packageBuild(buildDir) {
  console.log('📦 Packaging build results...');

  const distDir = join(buildDir, 'dist');
  console.log(`   Creating distribution package: ${distDir}`);

  execSync(`mkdir -p "${distDir}"`, { shell: '/bin/bash' });

  // Find executable
  const exeFiles = ['uxplay', 'uxplay.exe'];
  let exePath = null;

  for (const exe of exeFiles) {
    const fullPath = join(buildDir, exe);
    if (existsSync(fullPath)) {
      exePath = fullPath;
      break;
    }
  }

  if (!exePath) {
    console.warn('⚠️  Executable not found in build directory');
    return;
  }

  // Copy executable
  const exeName = exePath.split('/').pop();
  console.log(`   Copying ${exeName}...`);
  execSync(`cp "${exePath}" "${distDir}/"`, { shell: '/bin/bash' });

  // Copy documentation
  const docs = ['README.md', 'LICENSE'];
  for (const doc of docs) {
    if (existsSync(doc)) {
      console.log(`   Copying ${doc}...`);
      execSync(`cp "${doc}" "${distDir}/"`, { shell: '/bin/bash' });
    }
  }

  // Create build info
  const buildInfo = `UxPlay Windows Build
====================
Build Date: ${new Date().toISOString()}
Build Host: ${execSync('uname -a', { encoding: 'utf8' }).trim()}

Requirements:
- Bonjour SDK for Windows (Bonjour64.dll)
- GStreamer 1.14+ runtime
- libplist
- OpenSSL

Note: Cross-compiled from WSL using MinGW-w64
`;
  execSync(`echo '${buildInfo}' > "${distDir}/BUILD_INFO.txt"`, { shell: '/bin/bash' });

  console.log('✅ Package created successfully\n');
  console.log('📦 Package contents:');
  execSync(`ls -la "${distDir}"`, { stdio: 'inherit', shell: '/bin/bash' });
}

/**
 * Main command handler
 */
export async function command(buildWindows, args = []) {
  const { cli } = buildWindows;

  cli.section('Build UxPlay Windows EXE');

  // Parse arguments
  let sourcePath = '.';
  let clean = false;
  let dependenciesOnly = false;
  let verbose = false;

  for (let i = 0; i < args.length; i++) {
    const arg = args[i];
    if (arg === '--clean') {
      clean = true;
    } else if (arg === '--dependencies') {
      dependenciesOnly = true;
    } else if (arg === '--verbose') {
      verbose = true;
    } else if (arg.startsWith('-')) {
      cli.error(`Unknown option: ${arg}`);
      return;
    } else {
      sourcePath = arg;
    }
  }

  try {
    // Install dependencies only
    if (dependenciesOnly) {
      await installDependencies(verbose);
      return;
    }

    // Clean build
    if (clean) {
      await cleanBuild();
      if (args.length === 1 && args[0] === '--clean') {
        return;
      }
    }

    // Full build
    await installDependencies(verbose);
    await buildUxPlay(sourcePath, verbose);

    cli.success('✅ UxPlay Windows build completed successfully!');
    cli.info('');
    cli.info('Next steps:');
    cli.info('1. Copy the executable from build/dist/ to Windows');
    cli.info('2. Install GStreamer runtime (https://gstreamer.freedesktop.org/download/)');
    cli.info('3. Install Bonjour SDK (https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml)');
    cli.info('4. Run uxplay.exe on Windows');
    cli.info('');
    cli.info('For detailed instructions, see: build_windows_README.md');

  } catch (error) {
    cli.error('❌ Build failed');
    cli.error(error.message);
    process.exit(1);
  }
}
