import 'package:flutter/material.dart';
import 'package:table_calendar/table_calendar.dart';
import 'package:fl_chart/fl_chart.dart';

enum VisitType { 
  kupa, 
  siku, 
  nieznany
}

class CatVisit {
  final String id;
  final DateTime startTime;
  final DateTime endTime;
  final Duration duration;
  final VisitType type;
  final String? photoUrl;

  CatVisit({
    required this.id,
    required this.startTime,
    required this.endTime,
    required this.duration,
    required this.type,
    this.photoUrl,
  });
}

final List<CatVisit> mockVisits = [
  CatVisit(
    id: '1',
    startTime: DateTime.now().subtract(const Duration(hours: 2)),
    endTime: DateTime.now().subtract(const Duration(hours: 2)).add(const Duration(minutes: 3, seconds: 20)),
    duration: const Duration(minutes: 3, seconds: 20),
    type: VisitType.kupa,
  ),
  CatVisit(
    id: '2',
    startTime: DateTime.now().subtract(const Duration(hours: 5)),
    endTime: DateTime.now().subtract(const Duration(hours: 5)).add(const Duration(minutes: 1, seconds: 10)),
    duration: const Duration(minutes: 1, seconds: 10),
    type: VisitType.siku,
  ),
  CatVisit(
    id: '3',
    startTime: DateTime.now().subtract(const Duration(hours: 8)),
    endTime: DateTime.now().subtract(const Duration(hours: 8)).add(const Duration(minutes: 2, seconds: 45)),
    duration: const Duration(minutes: 2, seconds: 45),
    type: VisitType.siku,
  ),
  CatVisit(
    id: '4',
    startTime: DateTime.now().subtract(const Duration(days: 1, hours: 1)),
    endTime: DateTime.now().subtract(const Duration(days: 1, hours: 1)).add(const Duration(minutes: 4, seconds: 5)),
    duration: const Duration(minutes: 4, seconds: 5),
    type: VisitType.kupa,
  ),
];

void main() {
  runApp(const SmartKuweta());
}

class SmartKuweta extends StatelessWidget {
  const SmartKuweta({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Smart Kuweta',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme(
        brightness: Brightness.light,
        primary: Color(0xFF2C2C2C),        // ciemna szarość (jak ciemne futro)
        onPrimary: Colors.white,
        secondary: Color(0xFF757575),      // średnia szarość
        onSecondary: Colors.white,
        surface: Color(0xFFF5F5F5),        // jasna szarość (jak białe futro)
        onSurface: Color(0xFF2C2C2C),
        error: Color(0xFFB00020),
        onError: Colors.white,
  ),
        useMaterial3: true,
      ),
      home: const HomeScreen(),
    );
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _currentIndex = 0;

  final List<Widget> _screens = [
    const DashboardScreen(),
    const LogsScreen(),
    const StatsScreen(),
  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: _screens[_currentIndex],
      bottomNavigationBar: BottomNavigationBar(
        currentIndex: _currentIndex,
        onTap: (index) => setState(() => _currentIndex = index),
        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.home),
            label: 'Home',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.list),
            label: 'Logi',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.bar_chart),
            label: 'Statystyki',
          ),
        ],
      ),
    );
  }
}

class DashboardScreen extends StatelessWidget {
  const DashboardScreen({super.key});

  String _formatTime(DateTime dt) {
    return '${dt.hour.toString().padLeft(2, '0')}:${dt.minute.toString().padLeft(2, '0')}';
  }

  String _formatDate(DateTime dt) {
    final now = DateTime.now();
    if (dt.day == now.day) return 'Dzisiaj';
    if (dt.day == now.day - 1) return 'Wczoraj';
    return '${dt.day}.${dt.month}.${dt.year}';
  }

  String _formatDuration(Duration d) {
    return '${d.inMinutes}m ${d.inSeconds % 60}s';
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: const Color(0xFF2C2C2C),
        title: const Text(
          '🐱 Sven – Ostatnie aktywności',
          style: TextStyle(color: Colors.white, fontSize: 18),
        ),
      ),
      body: ListView.separated(
        padding: const EdgeInsets.all(16),
        itemCount: mockVisits.length,
        separatorBuilder: (_, __) => const Divider(),
        itemBuilder: (context, index) {
          final visit = mockVisits[index];
          final isKupa = visit.type == VisitType.kupa;
          return ListTile(
            leading: CircleAvatar(
              backgroundColor: isKupa
                  ? const Color(0xFF757575)
                  : const Color(0xFFB0BEC5),
              child: Text(
                isKupa ? '💩' : '💧',
                style: const TextStyle(fontSize: 20),
              ),
            ),
            title: Text(
              '${_formatDate(visit.startTime)} o ${_formatTime(visit.startTime)}',
              style: const TextStyle(fontWeight: FontWeight.bold),
            ),
            subtitle: Text('Czas: ${_formatDuration(visit.duration)}'),
            trailing: const Icon(Icons.chevron_right),
            onTap: () {
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (_) => VisitDetailScreen(visit: visit),
                ),
              );
            },
          );
        },
      ),
    );
  }
}

class VisitDetailScreen extends StatelessWidget {
  final CatVisit visit;
  const VisitDetailScreen({super.key, required this.visit});

  String _formatDuration(Duration d) {
    return '${d.inMinutes} minut ${d.inSeconds % 60} sekund';
  }

  @override
  Widget build(BuildContext context) {
    final isKupa = visit.type == VisitType.kupa;
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: const Color(0xFF2C2C2C),
        iconTheme: const IconThemeData(color: Colors.white),
        title: const Text(
          'Szczegóły wizyty',
          style: TextStyle(color: Colors.white),
        ),
      ),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Container(
              width: double.infinity,
              height: 200,
              decoration: BoxDecoration(
                color: const Color(0xFFF5F5F5),
                borderRadius: BorderRadius.circular(12),
              ),
              child: const Center(
                child: Text('📷 Zdjęcie z wizyty',
                    style: TextStyle(color: Colors.grey)),
              ),
            ),
            const SizedBox(height: 24),
            _infoRow('Typ', isKupa ? '💩 Kupa' : '💧 Siku'),
            _infoRow('Czas wizyty',
                '${visit.startTime.day}.${visit.startTime.month}.${visit.startTime.year} '
                '${visit.startTime.hour.toString().padLeft(2, '0')}:${visit.startTime.minute.toString().padLeft(2, '0')}'),
            _infoRow('Koniec wizyty',
                '${visit.endTime.hour.toString().padLeft(2, '0')}:${visit.endTime.minute.toString().padLeft(2, '0')}'),
            _infoRow('Czas trwania', _formatDuration(visit.duration)),
          ],
        ),
      ),
    );
  }

  Widget _infoRow(String label, String value) {
  return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        children: [
          Text('$label: ',
              style: const TextStyle(
                  fontWeight: FontWeight.bold,
                  fontSize: 16,
                  color: Color(0xFF2C2C2C))),
          Text(value,
              style: const TextStyle(
                  fontSize: 16, color: Color(0xFF757575))),
        ],
      ),
    );
  }
}

class LogsScreen extends StatefulWidget {
  const LogsScreen({super.key});

  @override
  State<LogsScreen> createState() => _LogsScreenState();
}

class _LogsScreenState extends State<LogsScreen> {
  DateTime _focusedDay = DateTime.now();
  DateTime? _selectedDay;

  List<CatVisit> _getVisitsForDay(DateTime day) {
    return mockVisits.where((visit) =>
        visit.startTime.year == day.year &&
        visit.startTime.month == day.month &&
        visit.startTime.day == day.day).toList();
  }

  @override
  Widget build(BuildContext context) {
    final selectedVisits = _selectedDay != null
        ? _getVisitsForDay(_selectedDay!)
        : [];

    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: const Color(0xFF2C2C2C),
        title: const Text(
          '📋 Logi',
          style: TextStyle(color: Colors.white),
        ),
      ),
      body: Column(
        children: [
          TableCalendar(
            firstDay: DateTime.utc(2024, 1, 1),
            lastDay: DateTime.utc(2026, 12, 31),
            focusedDay: _focusedDay,
            selectedDayPredicate: (day) => isSameDay(_selectedDay, day),
            onDaySelected: (selectedDay, focusedDay) {
              setState(() {
                _selectedDay = selectedDay;
                _focusedDay = focusedDay;
              });
            },
            eventLoader: (day) => _getVisitsForDay(day),
            calendarStyle: const CalendarStyle(
              todayDecoration: BoxDecoration(
                color: Color(0xFF757575),
                shape: BoxShape.circle,
              ),
              selectedDecoration: BoxDecoration(
                color: Color(0xFF2C2C2C),
                shape: BoxShape.circle,
              ),
              markerDecoration: BoxDecoration(
                color: Color(0xFF2C2C2C),
                shape: BoxShape.circle,
              ),
            ),
            headerStyle: const HeaderStyle(
              formatButtonVisible: false,
              titleCentered: true,
            ),
          ),
          const Divider(),
          if (_selectedDay == null)
            const Padding(
              padding: EdgeInsets.all(24),
              child: Text(
                'Kliknij dzień aby zobaczyć aktywność Svena',
                style: TextStyle(color: Colors.grey),
              ),
            )
          else if (selectedVisits.isEmpty)
            const Padding(
              padding: EdgeInsets.all(24),
              child: Text(
                'Brak aktywności w tym dniu 😴',
                style: TextStyle(color: Colors.grey),
              ),
            )
          else
            Expanded(
              child: ListView.builder(
                padding: const EdgeInsets.all(16),
                itemCount: selectedVisits.length,
                itemBuilder: (context, index) {
                  final visit = selectedVisits[index];
                  final isKupa = visit.type == VisitType.kupa;
                  return ListTile(
                    leading: Text(
                      isKupa ? '💩' : '💧',
                      style: const TextStyle(fontSize: 24),
                    ),
                    title: Text(
                      '${visit.startTime.hour.toString().padLeft(2, '0')}:${visit.startTime.minute.toString().padLeft(2, '0')} – '
                      '${visit.endTime.hour.toString().padLeft(2, '0')}:${visit.endTime.minute.toString().padLeft(2, '0')}',
                      style: const TextStyle(fontWeight: FontWeight.bold),
                    ),
                    subtitle: Text(
                      '${visit.duration.inMinutes}m ${visit.duration.inSeconds % 60}s',
                    ),
                    onTap: () {
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (_) => VisitDetailScreen(visit: visit),
                        ),
                      );
                    },
                  );
                },
              ),
            ),
        ],
      ),
    );
  }
}

class StatsScreen extends StatelessWidget {
  const StatsScreen({super.key});

  Map<int, int> _visitsPerHour() {
    final map = <int, int>{};
    for (final visit in mockVisits) {
      final hour = visit.startTime.hour;
      map[hour] = (map[hour] ?? 0) + 1;
    }
    return map;
  }

  Duration _averageDuration() {
    if (mockVisits.isEmpty) return Duration.zero;
    final total = mockVisits.fold<int>(
        0, (sum, v) => sum + v.duration.inSeconds);
    return Duration(seconds: total ~/ mockVisits.length);
  }

  int _countType(VisitType type) =>
      mockVisits.where((v) => v.type == type).length;

  @override
  Widget build(BuildContext context) {
    final visitsPerHour = _visitsPerHour();
    final avgDuration = _averageDuration();

    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: const Color(0xFF2C2C2C),
        title: const Text(
          '📊 Statystyki',
          style: TextStyle(color: Colors.white),
        ),
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Karty podsumowania
            Row(
              children: [
                Expanded(
                  child: _statCard(
                    '🐱 Wizyty',
                    '${mockVisits.length}',
                    'łącznie',
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _statCard(
                    '⏱️ Średni czas',
                    '${avgDuration.inMinutes}m ${avgDuration.inSeconds % 60}s',
                    'na wizytę',
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  child: _statCard(
                    '💩 Kupy',
                    '${_countType(VisitType.kupa)}',
                    'łącznie',
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _statCard(
                    '💧 Siku',
                    '${_countType(VisitType.siku)}',
                    'łącznie',
                  ),
                ),
              ],
            ),
            const SizedBox(height: 24),
            // Wykres
            const Text(
              'Aktywność według godziny',
              style: TextStyle(
                fontSize: 16,
                fontWeight: FontWeight.bold,
                color: Color(0xFF2C2C2C),
              ),
            ),
            const SizedBox(height: 12),
            SizedBox(
              height: 200,
              child: BarChart(
                BarChartData(
                  alignment: BarChartAlignment.spaceAround,
                  maxY: 3,
                  barTouchData: BarTouchData(enabled: false),
                  titlesData: FlTitlesData(
                    leftTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 28,
                        interval: 1,
                        getTitlesWidget: (value, meta) {
                          if (value == value.roundToDouble()) {
                            return Text(
                              value.toInt().toString(),
                              style: const TextStyle(fontSize: 10),
                            );
                          }
                          return const SizedBox.shrink();
                        },
                      ),
                    ),
                    bottomTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        getTitlesWidget: (value, meta) => Padding(
                          padding: const EdgeInsets.only(top: 4),
                          child: Text(
                            '${value.toInt().toString().padLeft(2, '0')}:00',
                            style: const TextStyle(fontSize: 9),
                          ),
                        ),
                      ),
                    ),
                    topTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false)),
                    rightTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false)),
                  ),
                  gridData: FlGridData(
                    show: true,
                    horizontalInterval: 1,
                    getDrawingHorizontalLine: (value) => FlLine(
                      color: Colors.grey.shade200,
                      strokeWidth: 1,
                    ),
                    drawVerticalLine: false,
                  ),
                  borderData: FlBorderData(show: false),
                  barGroups: List.generate(24, (hour) {
                    final count = visitsPerHour[hour] ?? 0;
                    return BarChartGroupData(
                      x: hour,
                      barRods: [
                        BarChartRodData(
                          toY: count.toDouble(),
                          color: count > 0
                              ? const Color(0xFF2C2C2C)
                              : Colors.grey.shade200,
                          width: 8,
                          borderRadius: BorderRadius.circular(4),
                        ),
                      ],
                    );
                  }),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _statCard(String title, String value, String subtitle) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFFF5F5F5),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(title,
              style: const TextStyle(
                  fontSize: 14, color: Color(0xFF757575))),
          const SizedBox(height: 8),
          Text(value,
              style: const TextStyle(
                  fontSize: 24,
                  fontWeight: FontWeight.bold,
                  color: Color(0xFF2C2C2C))),
          Text(subtitle,
              style: const TextStyle(
                  fontSize: 12, color: Color(0xFF757575))),
        ],
      ),
    );
  }
}