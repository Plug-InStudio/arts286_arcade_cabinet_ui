import 'package:flutter/material.dart';
import 'dart:io';
import 'package:process_run/process_run.dart';
import 'package:flutter/services.dart';
import 'package:window_manager/window_manager.dart';

final String executables_path = "..\\executables\\";

final Color game_select_col = Color.fromARGB(255, 255, 255, 0);
final Color game_unselect_col = Color.fromARGB(255, 255, 255, 255);
final Color info_text_col = Color.fromARGB(255, 0, 255, 255);

Future<void> openExeFile(String pathToExe) async {
  final exeFile = File(pathToExe);

  if (await exeFile.exists()) {
    try {
      final result = await run(exeFile.path);
      // print('Executable output:\n${result.stdout}');
    } catch (e) {
      print('Error running exe: $e');
    }
  } else {
    print('Executable not found at: ${exeFile.path}');
  }
}

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  // Must add this line.
  await windowManager.ensureInitialized();

  WindowOptions windowOptions = WindowOptions(
    backgroundColor: Colors.transparent,
    skipTaskbar: false,
    fullScreen: true,
    titleBarStyle: TitleBarStyle.hidden,
  );

  windowManager.waitUntilReadyToShow(windowOptions, () async {
    await windowManager.show();
    await windowManager.focus();
  });

  runApp(
    const MouseRegion(
      cursor: SystemMouseCursors.none, // Hide mouse cursor
      child: ArtcadeApp(),
    ),
  );
}

class ArtcadeApp extends StatelessWidget {
  const ArtcadeApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'S2DIO 286 ARTCADE',
      home: const ArcadeHomePage(),
      debugShowCheckedModeBanner: false,
    );
  }
}

class ArcadeHomePage extends StatefulWidget {
  const ArcadeHomePage({super.key});

  @override
  State<ArcadeHomePage> createState() => _ArcadeHomePageState();
}

class _ArcadeHomePageState extends State<ArcadeHomePage> {
  List<String> games = [];
  int selectedIndex = 0;
  String? selectedGameImagePath;
  @override
  void initState() {
    super.initState();
    _loadGameFolders();
    RawKeyboard.instance.addListener(_handleKey);
  }

  @override
  void dispose() {
    RawKeyboard.instance.removeListener(_handleKey);
    super.dispose();
  }

  void _loadGameFolders() async {
    final directory = Directory('$executables_path');
    if (await directory.exists()) {
      final folders = directory.listSync().whereType<Directory>();
      setState(() {
        games =
            folders
                .map((d) => d.path.split(Platform.pathSeparator).last)
                .toList();
        if (games.isEmpty) games = ['No games found'];
      });
      _updateGameImage();
    } else {
      setState(() {
        games = ['Game folder not found'];
      });
    }
  }

  void _moveSelectionUp() {
    setState(() {
      selectedIndex = (selectedIndex - 1 + games.length) % games.length;
    });
    _updateGameImage();
  }

  void _moveSelectionDown() {
    setState(() {
      selectedIndex = (selectedIndex + 1) % games.length;
    });
    _updateGameImage();
  }

  void _handleKey(RawKeyEvent event) {
    if (event is RawKeyDownEvent) {
      final key = event.logicalKey;

      if (key == LogicalKeyboardKey.arrowUp || key == LogicalKeyboardKey.keyA) {
        _moveSelectionUp();
      } else if (key == LogicalKeyboardKey.arrowDown ||
          key == LogicalKeyboardKey.keyD) {
        _moveSelectionDown();
      } else if ([
        LogicalKeyboardKey.keyZ,
        LogicalKeyboardKey.keyX,
        LogicalKeyboardKey.keyN,
        LogicalKeyboardKey.keyM,
      ].contains(key)) {
        _launchSelectedGame();
      }
    }
  }

  void _launchSelectedGame() async {
    if (games.isEmpty) return;
    final gameFolder = Directory('$executables_path${games[selectedIndex]}');

    if (await gameFolder.exists()) {
      final files = gameFolder.listSync(recursive: true);
      final exeFile = files.firstWhere(
        (f) => f is File && f.path.toLowerCase().endsWith('.exe'),
      );

      if (exeFile != null && exeFile is File) {
        try {
          await run(exeFile.path);
        } catch (e) {
          print('Error launching game: $e');
        }
      } else {
        print('No .exe file found in ${gameFolder.path}');
      }
    }
  }

  void _updateGameImage() async {
    selectedGameImagePath = null;
    if (games.isEmpty) return;
    final gameFolder = Directory('$executables_path${games[selectedIndex]}');
    if (await gameFolder.exists()) {
      final files = gameFolder.listSync(recursive: true);
      final imageFile = files.firstWhere(
        (f) =>
            f is File &&
            (f.path.toLowerCase().endsWith('.png') ||
                f.path.toLowerCase().endsWith('.jpg') ||
                f.path.toLowerCase().endsWith('.jpeg')),
        //orElse: () => null,
      );

      setState(() {
        selectedGameImagePath = imageFile is File ? imageFile.path : null;
      });
    }
  }

  List<String> getVisibleGames() {
    if (games.length < 3) {
      return List.generate(
        3,
        (i) => games[(selectedIndex + i - 1 + games.length) % games.length],
      );
    } else {
      return [
        games[(selectedIndex - 1 + games.length) % games.length],
        games[selectedIndex],
        games[(selectedIndex + 1) % games.length],
      ];
    }
  }

  @override
  Widget build(BuildContext context) {
    final visibleGames = getVisibleGames();

    return Scaffold(
      backgroundColor: Colors.black,
      body: SafeArea(
        child: Center(
          child: FittedBox(
            fit: BoxFit.fill,
            child: SizedBox(
              width: 1920,
              height: 1080,
              child: Stack(
                children: [
                  const Positioned(
                    top: 16,
                    left: 0,
                    right: 0,
                    child: Center(
                      child: Text(
                        'S2DIO S28 ARTCADE',
                        style: TextStyle(
                          fontSize: 28,
                          fontWeight: FontWeight.bold,
                          color: Colors.white,
                        ),
                      ),
                    ),
                  ),
                  Positioned(
                    left: 100,
                    top: 400,
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children:
                          visibleGames.map((game) {
                            final index = games.indexOf(game);
                            final isSelected = index == selectedIndex;
                            return Padding(
                              padding: const EdgeInsets.symmetric(
                                vertical: 8.0,
                              ),
                              child: Row(
                                children: [
                                  if (isSelected)
                                    const Icon(
                                      Icons.arrow_right,
                                      color: Colors.yellow,
                                    )
                                  else
                                    const SizedBox(width: 24),
                                  Text(
                                    game,
                                    style: TextStyle(
                                      fontSize: 20,
                                      color:
                                          isSelected
                                              ? Colors.yellow
                                              : Colors.white,
                                      fontWeight:
                                          isSelected
                                              ? FontWeight.bold
                                              : FontWeight.normal,
                                    ),
                                  ),
                                ],
                              ),
                            );
                          }).toList(),
                    ),
                  ),
                  Positioned(
                    left: 820,
                    top: 258.75,
                    width: 1000,
                    height: 562.5,
                    child: Container(
                      color: Colors.grey.shade800,
                      child:
                          selectedGameImagePath != null
                              ? Image.file(
                                File(selectedGameImagePath!),
                                fit: BoxFit.cover,
                              )
                              : Center(
                                child: Text(
                                  games.isNotEmpty ? games[selectedIndex] : '',
                                  style: const TextStyle(color: Colors.white),
                                ),
                              ),
                    ),
                  ),
                  const Positioned(
                    bottom: 16,
                    left: 0,
                    right: 0,
                    child: Center(
                      child: Text(
                        'Games made by students',
                        style: TextStyle(fontSize: 16, color: Colors.white),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}
