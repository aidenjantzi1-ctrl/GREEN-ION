# Supabase Private Cloud Backup Guide

GREEN ION saves original **BIN, SEN and TXT** files to the browser first. Supabase is a secondary user-controlled backup, not a requirement for recording.

1. Open [V14](https://green-ion-beta.vercel.app/) → **Cloud** and create an email/password account or sign in. Confirm the email if requested.
2. In **System**, leave **Automatic cloud sync** enabled for background upload when internet is usable. Turn it off whenever you prefer manual-only backups.
3. Use **Back up local recordings now** to start a manual backup and view per-file progress, number stored and approximate usage.
4. On another device, sign into the same account and select **Restore cloud recordings**. Only records that pass SHA-256 and size checks are saved; existing originals are not overwritten.
5. Keep an offline exported archive and the SD card as independent backups.

**Reliability:** cloud listings are paginated; object keys use each user's ID and the original checksum; duplicates are avoided; retries back off up to 30 minutes after errors. Automatic upload stops in Data Saver mode and resumes when internet returns. Logging out never deletes the original local recordings.

**Security:** bucket green-ion-flights is private; metadata table flight_backups and storage paths have owner-scoped RLS policies. The browser only contains a public publishable key, not an administrator key. Sign-in tokens are held in local browser storage; sign out on shared computers. GPS GeoJSON can contain sensitive real-world positions.

**Not yet physically verified:** account registration, email confirmation, full upload/restore and token renewal need an end-to-end test using actual devices and an account. Database security checks alone are not a complete live validation.

Sources: [Supabase Auth](https://supabase.com/docs/guides/auth), [Storage security](https://supabase.com/docs/guides/storage/security/access-control), [RLS](https://supabase.com/docs/guides/database/postgres/row-level-security).
