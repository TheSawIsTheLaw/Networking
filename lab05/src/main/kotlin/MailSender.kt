import javax.mail.*
import javax.mail.internet.InternetAddress
import javax.mail.internet.MimeMessage

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
                    "4. Key word for attached text file."
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
        message.setText(messageText)

        Transport.send(message)

        println("Sent successfully")
    }
    catch (exc: Exception)
    {
        println("Your message can't be sent :(")
    }

    return
}