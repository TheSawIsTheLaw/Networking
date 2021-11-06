import java.io.BufferedReader
import java.io.File
import java.nio.charset.Charset
import java.nio.file.Paths
import javax.mail.*
import javax.mail.internet.*

fun findFilesToAttach(keyWord: String): List<File>
{
    val pathOfFilesToBeAttached = "textFilesToBeAttached/"
    val filesToInspect = mutableListOf<File>()
    File(pathOfFilesToBeAttached).walk().forEach {
        if (it.name.contains(".txt")) filesToInspect.add(it)
    }

    val filesToAttach = mutableListOf<File>()
    val lowercaseKey = keyWord.lowercase()
    for (file in filesToInspect)
    {
        val fileString = file.bufferedReader().use { it.readText() }.lowercase()
        if (fileString.contains(lowercaseKey))
        {
            filesToAttach.add(file)
        }
    }

    return filesToAttach
}

fun main(args: Array<String>)
{
    if (args.size != 5)
    {
        println(
            "Parameters:\n" +
                    "1. Recipient address\n" +
                    "2. Sender address\n" +
                    "3. Sender password\n" +
                    "4. Message\n" +
                    "5. Key word for attached text file."
        )

        return
    }

    val recipientAddress = args[0]
    val senderAddress = args[1]
    val senderPassword = args[2]
    val messageText = args[3]
    val keyWordToAttachFile = args[4]

    val host = "smtp.gmail.com"

    val properties = System.getProperties()

    properties["mail.smtp.host"] = host
    properties["mail.smtp.port"] = "465"
    properties["mail.smtp.ssl.enable"] = true
    properties["mail.smtp.auth"] = true

    val session = Session.getInstance(properties, object : Authenticator()
    {
        override fun getPasswordAuthentication(): PasswordAuthentication
        {
            return PasswordAuthentication(senderAddress, senderPassword)
        }
    })

    session.debug = true

    try
    {
        val message = MimeMessage(session)

        message.setFrom(InternetAddress(senderAddress))
        message.addRecipient(
            Message.RecipientType.TO,
            InternetAddress(recipientAddress)
        )
        message.subject = "Looks like i'm using some strange ways to write " +
                "you a message..."
        val multipart = MimeMultipart()

        val textPart = MimeBodyPart()
        textPart.setText(messageText)
        multipart.addBodyPart(textPart)

        for (file in findFilesToAttach(keyWordToAttachFile))
        {
            val filePart = MimeBodyPart()
            filePart.attachFile(file)
            multipart.addBodyPart(filePart)
        }

        message.setContent(multipart)
        Transport.send(message)

        println("Sent successfully")
    }
    catch (exc: Exception)
    {
        println("Your message can't be sent :(")
    }
}