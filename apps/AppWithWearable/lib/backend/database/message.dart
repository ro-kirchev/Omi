import 'package:friend_private/backend/database/memory.dart';
import 'package:friend_private/backend/preferences.dart';
import 'package:objectbox/objectbox.dart';

enum MessageSender { ai, human }

enum MessageType { text, daySummary }

@Entity()
class Message {
  @Id()
  int id = 0;

  @Index()
  @Property(type: PropertyType.date)
  DateTime createdAt;

  String text;
  String sender;
  String? pluginId;

  set senderEnum(MessageSender sender) => this.sender = sender.toString().split('.').last;

  MessageSender get senderEnum => MessageSender.values.firstWhere((e) => e.toString().split('.').last == sender);

  String type;

  set typeEnum(MessageType type) => this.type = type.toString().split('.').last;

  MessageType get typeEnum => MessageType.values.firstWhere((e) => e.toString().split('.').last == type);

  final memories = ToMany<Memory>();

  Message(this.createdAt, this.text, this.sender, {this.id = 0, this.type = 'text', this.pluginId});

  static String getMessagesAsString(List<Message> messages, {bool useUserNameIfAvailable = false}) {
    var sortedMessages = messages.toList()..sort((a, b) => a.createdAt.compareTo(b.createdAt));
    return sortedMessages.map((e) {
      var sender = e.sender == 'human'
          ? SharedPreferencesUtil().givenName.isNotEmpty && useUserNameIfAvailable
              ? SharedPreferencesUtil().givenName
              : 'USER'
          : e.sender.toString().toUpperCase();
      return '(${e.createdAt.toIso8601String().split('.')[0]}) $sender: ${e.text}';
    }).join('\n');
  }
}
